# foo_dsp_cassette 設計書

## カセットテープ音色エミュレーション DSP コンポーネント for foobar2000

---

## 1. 概要

### 1.1 目的

foobar2000 上で再生中の音源に対し、カセットテープ特有の音響特性をリアルタイムに付加する DSP コンポーネントを開発する。

### 1.2 コンポーネント情報

| 項目 | 値 |
|------|------|
| コンポーネント名 | Cassette Tape Emulator |
| ファイル名 | foo_dsp_cassette.dll |
| 対象プラットフォーム | Windows 64-bit（32-bit も可） |
| 開発言語 | C++17 |
| 依存 SDK | foobar2000 SDK + WTL（UI） |
| DSP GUID | 自動生成（実装時に固定） |

### 1.3 機能一覧

| # | 機能 | 概要 |
|---|------|------|
| 1 | ローパスフィルター | テープの高域減衰を再現 |
| 2 | テープヒス | 「サー」というノイズの付加 |
| 3 | ワウ＆フラッター | モーター回転ムラによるピッチ揺れ |
| 4 | サチュレーション | テープ磁気飽和による倍音付加・ソフトクリッピング |

---

## 2. アーキテクチャ

### 2.1 処理チェーン

入力信号は以下の順序で直列に処理する。

```
入力 PCM
  │
  ├─① サチュレーション（波形の非線形変換）
  │
  ├─② ローパスフィルター（高域カット）
  │
  ├─③ ワウ＆フラッター（ピッチ変調）
  │
  ├─④ テープヒス（ノイズ加算）
  │
  ▼
出力 PCM
```

**処理順序の根拠：**

- サチュレーションを最初に置くことで、原音に対して倍音を付加し、後段のフィルターで不要な高域倍音が自然に丸められる。
- ローパスフィルターをワウ＆フラッターの前に置くことで、ピッチ変調時に高域のエイリアシングが抑制される。
- テープヒスは最後に加算する。実機ではヒスは再生系で発生するため、他の処理の影響を受けない。

### 2.2 クラス構成

```
foo_dsp_cassette (dsp_impl_base 継承)
├── CassetteSaturation     ... サチュレーション処理
├── CassetteLowPass        ... ローパスフィルター処理
├── CassetteWowFlutter     ... ワウ＆フラッター処理
├── CassetteTapeHiss       ... テープヒスノイズ生成
└── CassetteConfig         ... パラメータ管理・UI連携
```

### 2.3 SDK 基底クラス

```cpp
class foo_dsp_cassette : public dsp_impl_base {
public:
    // DSP メタ情報
    static GUID g_get_guid();
    static void g_get_name(pfc::string_base & out);

    // 初期化・終了
    void on_init(unsigned p_flags) override;

    // チャンク処理（メインループ）
    void on_chunk(audio_chunk * chunk) override;

    // リセット
    void on_endoftrack(abort_callback & p_abort) override;
    void on_endofplayback(abort_callback & p_abort) override;

    // コンフィグ
    bool get_config(dsp_preset & p_out) override;
    static bool parse_config(const dsp_preset & preset, CassetteConfig & cfg);
};
```

---

## 3. 各モジュール詳細設計

### 3.1 ローパスフィルター（CassetteLowPass）

#### 3.1.1 目的

カセットテープの周波数応答を模擬する。実テープでは高域が緩やかに減衰する。

#### 3.1.2 ユーザーパラメータ

| パラメータ | 変数名 | 型 | 範囲 | デフォルト | 説明 |
|-----------|--------|-----|------|-----------|------|
| カットオフ周波数 | `lpf_cutoff_hz` | float | 2000 – 20000 Hz | 12000 Hz | -3dB ポイント。下げるほどこもった音になる |

#### 3.1.3 アルゴリズム

2次 IIR バイクアッドフィルター（Butterworth 特性）を使用する。

```
フィルター伝達関数（Direct Form II Transposed）:
  y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
```

**係数計算：**

```cpp
void calc_coefficients(float sample_rate, float cutoff_hz) {
    float omega = 2.0f * M_PI * cutoff_hz / sample_rate;
    float sin_omega = sinf(omega);
    float cos_omega = cosf(omega);
    float alpha = sin_omega / (2.0f * Q);  // Q = 0.7071 (Butterworth)

    float a0 = 1.0f + alpha;
    b0 = ((1.0f - cos_omega) / 2.0f) / a0;
    b1 = (1.0f - cos_omega) / a0;
    b2 = b0;
    a1 = (-2.0f * cos_omega) / a0;
    a2 = (1.0f - alpha) / a0;
}
```

#### 3.1.4 内部状態

| 変数 | 型 | 説明 |
|------|-----|------|
| `x1, x2` | float（チャンネルごと） | 入力の遅延バッファ |
| `y1, y2` | float（チャンネルごと） | 出力の遅延バッファ |
| `b0, b1, b2, a1, a2` | float | フィルター係数 |

#### 3.1.5 備考

- カットオフ周波数またはサンプルレートが変更された場合、係数を再計算する。
- チャンネルごとに独立した遅延バッファを保持する（ステレオ対応）。

---

### 3.2 テープヒス（CassetteTapeHiss）

#### 3.2.1 目的

テープ再生時の「サー」というバックグラウンドノイズを付加する。

#### 3.2.2 ユーザーパラメータ

| パラメータ | 変数名 | 型 | 範囲 | デフォルト | 説明 |
|-----------|--------|-----|------|-----------|------|
| オン/オフ | `hiss_enabled` | bool | true / false | true | ヒスノイズの有効・無効 |
| レベル | `hiss_level_db` | float | -60 – -20 dB | -40 dB | ノイズの音量（将来拡張用。初期実装では固定でもよい） |

#### 3.2.3 アルゴリズム

1. 一様乱数から白色雑音を生成する。
2. 簡易ローパス（1次 IIR、カットオフ約 8kHz）でピンクノイズに近づける。テープヒスは高域寄りだが超高域は減衰するため。
3. 振幅を `hiss_level_db` に応じてスケーリングし、原音に加算する。

```cpp
// 白色雑音生成（-1.0 ～ +1.0）
float white = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;

// 簡易ローパスで帯域制限（テープヒスらしい帯域に整形）
hiss_state = hiss_state * 0.85f + white * 0.15f;

// レベル適用
float hiss_gain = powf(10.0f, hiss_level_db / 20.0f);
output = input + hiss_state * hiss_gain;
```

#### 3.2.4 内部状態

| 変数 | 型 | 説明 |
|------|-----|------|
| `hiss_state` | float（チャンネルごと） | ノイズフィルタの状態変数 |
| `rng` | std::mt19937 | 乱数生成器（rand() より品質が良い） |

#### 3.2.5 備考

- `std::mt19937` + `std::uniform_real_distribution` を使用し、音質の良い乱数を生成する。
- 将来的にはノイズの帯域をより精密に制御する拡張が可能。

---

### 3.3 ワウ＆フラッター（CassetteWowFlutter）

#### 3.3.1 目的

テープ駆動モーターの回転ムラに起因するピッチの微小変動を再現する。

#### 3.3.2 ユーザーパラメータ

| パラメータ | 変数名 | 型 | 範囲 | デフォルト | 説明 |
|-----------|--------|-----|------|-----------|------|
| Rate | `wf_rate_hz` | float | 0.5 – 15.0 Hz | 3.0 Hz | 揺れの基本周波数 |
| Depth | `wf_depth` | float | 0.0 – 1.0 | 0.3 | 揺れの深さ（最大ピッチ偏差の割合） |
| Randomness | `wf_randomness` | float | 0.0 – 1.0 | 0.5 | 揺れの不規則さ |

#### 3.3.3 アルゴリズム

**変調信号の生成：**

```
modulation = sin(2π * rate * t) * (1 - randomness) + noise(t) * randomness
delay_samples = base_delay + modulation * depth * max_delay
```

- `base_delay`：遅延バッファの中心位置（max_delay / 2）
- `max_delay`：サンプルレートに依存。最大ピッチ偏差 ±3% として `sample_rate * 0.03` 程度。
- `noise(t)`：ローパスフィルタ済みの乱数（帯域を rate 付近に制限）

**ピッチ変調の実装：**

可変長遅延線（fractional delay line）を用いる。

```cpp
// 遅延バッファへの書き込み
delay_buffer[write_pos] = input_sample;

// 変調量の計算
float lfo = sinf(2.0f * M_PI * wf_rate_hz * phase);
float noise = get_filtered_noise();  // LPF済み乱数
float mod = lfo * (1.0f - wf_randomness) + noise * wf_randomness;

// 読み出し位置（小数点以下あり）
float delay = base_delay + mod * wf_depth * max_delay_samples;
float read_pos = (float)write_pos - delay;

// 線形補間で読み出し
int idx0 = (int)floorf(read_pos);
float frac = read_pos - (float)idx0;
output = delay_buffer[wrap(idx0)] * (1.0f - frac)
       + delay_buffer[wrap(idx0 + 1)] * frac;
```

#### 3.3.4 内部状態

| 変数 | 型 | 説明 |
|------|-----|------|
| `delay_buffer` | std::vector\<float\>（チャンネルごと） | リングバッファ |
| `write_pos` | int | 書き込みポインタ |
| `phase` | float | LFO の位相（0.0 – 1.0） |
| `noise_state` | float | ランダム変調のフィルタ状態 |
| `max_delay_samples` | int | バッファサイズ（サンプルレート依存） |

#### 3.3.5 パラメータの効果ガイドライン

| 設定例 | Rate | Depth | Randomness | 音の印象 |
|--------|------|-------|------------|---------|
| 微かな揺れ | 1.0 | 0.1 | 0.3 | 高品質デッキ風 |
| 標準的カセット | 3.0 | 0.3 | 0.5 | 一般的なラジカセ風 |
| 劣化テープ | 6.0 | 0.7 | 0.8 | 伸びたテープ・安物プレーヤー風 |

#### 3.3.6 備考

- Rate が低い（< 2Hz）ときはワウ的、高い（> 6Hz）ときはフラッター的な揺れになる。
- 線形補間で十分な品質が得られるが、より高品質を求める場合は3次（Hermite / Catmull-Rom）補間に変更可能。

---

### 3.4 サチュレーション（CassetteSaturation）

#### 3.4.1 目的

テープの磁気飽和によるソフトクリッピング・倍音付加を再現する。

#### 3.4.2 ユーザーパラメータ

| パラメータ | 変数名 | 型 | 範囲 | デフォルト | 説明 |
|-----------|--------|-----|------|-----------|------|
| Drive | `sat_drive` | float | 1.0 – 10.0 | 2.0 | 入力ゲイン倍率。大きいほど歪む |
| Mix | `sat_mix` | float | 0.0 – 1.0 | 0.5 | ドライ / ウェット比 |

#### 3.4.3 アルゴリズム

`tanh` によるソフトクリッピングを使用する。

```cpp
float process(float input) {
    float driven = input * sat_drive;
    float saturated = tanhf(driven);

    // ゲイン補正（drive が大きいほど音量が下がるのを補正）
    float compensation = 1.0f / tanhf((float)sat_drive);
    saturated *= compensation;

    // ドライ・ウェットミックス
    return input * (1.0f - sat_mix) + saturated * sat_mix;
}
```

#### 3.4.4 波形変換の特性

```
入力振幅 →  tanh(drive * x) の出力

drive=1:  ほぼリニア（変化なし）
drive=2:  ±0.5 付近からわずかに圧縮が始まる
drive=5:  明確なソフトクリッピング、奇数次倍音が顕著
drive=10: 強い歪み、ほぼ矩形波に近づく
```

#### 3.4.5 内部状態

サチュレーションは状態を持たない純粋な関数（memoryless）。

#### 3.4.6 備考

- `tanh` は計算コストが比較的高いため、必要に応じて多項式近似に置換可能。
- ゲイン補正により、Drive を変化させても知覚音量がほぼ一定に保たれる。

---

## 4. パラメータ管理（CassetteConfig）

### 4.1 データ構造

```cpp
struct CassetteConfig {
    // ローパスフィルター
    float lpf_cutoff_hz   = 12000.0f;  // 2000 – 20000

    // テープヒス
    bool  hiss_enabled     = true;
    float hiss_level_db    = -40.0f;   // -60 – -20

    // ワウ＆フラッター
    float wf_rate_hz       = 3.0f;     // 0.5 – 15.0
    float wf_depth         = 0.3f;     // 0.0 – 1.0
    float wf_randomness    = 0.5f;     // 0.0 – 1.0

    // サチュレーション
    float sat_drive        = 2.0f;     // 1.0 – 10.0
    float sat_mix          = 0.5f;     // 0.0 – 1.0
};
```

### 4.2 シリアライズ

`dsp_preset` のバイナリデータとして保存する。バージョン番号を先頭に付加し、将来のパラメータ追加に備える。

```cpp
// 書き込み
void serialize(dsp_preset & out) {
    dsp_preset_builder builder;
    builder << (uint32_t)CONFIG_VERSION;
    builder << lpf_cutoff_hz;
    builder << hiss_enabled;
    builder << hiss_level_db;
    builder << wf_rate_hz;
    builder << wf_depth;
    builder << wf_randomness;
    builder << sat_drive;
    builder << sat_mix;
    builder.finish(g_get_guid(), out);
}

// 読み込み
bool deserialize(const dsp_preset & in) {
    dsp_preset_parser parser(in);
    uint32_t version;
    parser >> version;
    if (version > CONFIG_VERSION) return false;
    parser >> lpf_cutoff_hz;
    parser >> hiss_enabled;
    parser >> hiss_level_db;
    parser >> wf_rate_hz;
    parser >> wf_depth;
    parser >> wf_randomness;
    parser >> sat_drive;
    parser >> sat_mix;
    return true;
}
```

### 4.3 プリセット（将来拡張）

| プリセット名 | Cutoff | Hiss | Rate | Depth | Random | Drive | Mix |
|-------------|--------|------|------|-------|--------|-------|-----|
| High Quality Deck | 16000 | ON -50dB | 1.0 | 0.1 | 0.2 | 1.5 | 0.3 |
| Standard Cassette | 12000 | ON -40dB | 3.0 | 0.3 | 0.5 | 2.0 | 0.5 |
| Worn Out Tape | 8000 | ON -30dB | 5.0 | 0.6 | 0.8 | 3.5 | 0.7 |
| Lo-Fi Extreme | 4000 | ON -25dB | 8.0 | 0.9 | 0.9 | 6.0 | 0.9 |

---

## 5. UI 設計

### 5.1 設定ダイアログ

foobar2000 の DSP 設定画面（Preferences → Playback → DSP Manager）から「Configure」ボタンで開くモーダルダイアログとして実装する。

```
┌──────────────────────────────────────────────┐
│  Cassette Tape Emulator                      │
├──────────────────────────────────────────────┤
│                                              │
│  ── Low Pass Filter ──────────────────────── │
│  Cutoff:  [=====●================] 12000 Hz  │
│                                              │
│  ── Tape Hiss ────────────────────────────── │
│  [✓] Enable                                 │
│  Level:   [========●=============]  -40 dB   │
│                                              │
│  ── Wow & Flutter ────────────────────────── │
│  Rate:    [===●==================]  3.0 Hz   │
│  Depth:   [====●=================]  0.30     │
│  Random:  [========●=============]  0.50     │
│                                              │
│  ── Saturation ───────────────────────────── │
│  Drive:   [==●===================]  2.0x     │
│  Mix:     [========●=============]  50%      │
│                                              │
│          [ Reset Defaults ]                  │
│                        [ OK ]  [ Cancel ]    │
└──────────────────────────────────────────────┘
```

### 5.2 UI 実装方針

- Win32 ダイアログリソース（.rc）+ WTL を使用する。
- 各スライダーは `TRACKBAR_CLASS`（トラックバーコントロール）で実装する。
- スライダー操作時にリアルタイムで DSP パラメータを更新し、即座に音に反映される。
- 「Reset Defaults」ボタンで全パラメータを初期値に戻す。

---

## 6. ビルド構成

### 6.1 必要環境

| 項目 | バージョン |
|------|-----------|
| Visual Studio | 2022（v143 ツールセット） |
| foobar2000 SDK | 最新版（公式サイトより取得） |
| WTL | 10.0 以降 |
| C++ 標準 | C++17 |

### 6.2 プロジェクト構成

```
foo_dsp_cassette/
├── src/
│   ├── dsp_cassette.cpp         // DSP メインクラス（エントリポイント）
│   ├── dsp_cassette.h
│   ├── cassette_lowpass.h       // ローパスフィルター
│   ├── cassette_hiss.h          // テープヒス
│   ├── cassette_wow_flutter.h   // ワウ＆フラッター
│   ├── cassette_saturation.h    // サチュレーション
│   ├── cassette_config.h        // パラメータ構造体
│   └── cassette_dialog.cpp      // 設定ダイアログ
├── res/
│   └── cassette_dialog.rc       // ダイアログリソース
├── pch.h                        // プリコンパイル済みヘッダー
└── foo_dsp_cassette.vcxproj
```

### 6.3 ビルド設定の要点

- foobar2000 SDK のヘッダーおよびライブラリパスをインクルードディレクトリに追加。
- 出力形式は DLL（拡張子 `.fb2k-component` にリネーム）。
- x64 Release ビルドを基本とする。
- `/fp:fast` を有効にし浮動小数点演算を高速化する。

---

## 7. on_chunk 処理フロー

DSP のメイン処理ループである `on_chunk` の全体フローを以下に示す。

```cpp
void foo_dsp_cassette::on_chunk(audio_chunk * chunk) {
    // サンプルレート変更検知 → フィルター係数再計算
    unsigned sr = chunk->get_sample_rate();
    unsigned ch = chunk->get_channel_count();
    if (sr != current_sample_rate || ch != current_channels) {
        current_sample_rate = sr;
        current_channels = ch;
        lowpass.update_coefficients(sr, cfg.lpf_cutoff_hz);
        wow_flutter.update_sample_rate(sr);
    }

    audio_sample * data = chunk->get_data();
    t_size samples = chunk->get_sample_count();

    for (t_size i = 0; i < samples; i++) {
        for (unsigned c = 0; c < ch; c++) {
            t_size idx = i * ch + c;
            float sample = data[idx];

            // ① サチュレーション
            sample = saturation.process(sample, cfg.sat_drive, cfg.sat_mix);

            // ② ローパスフィルター
            sample = lowpass.process(sample, c);

            // ③ ワウ＆フラッター
            sample = wow_flutter.process(sample, c);

            // ④ テープヒス
            if (cfg.hiss_enabled) {
                sample = hiss.process(sample, c);
            }

            data[idx] = sample;
        }

        // LFO 位相更新（サンプルごと・全チャンネル共通）
        wow_flutter.advance_phase(sr);
    }
}
```

---

## 8. 既知の制約・注意事項

| # | 項目 | 内容 |
|---|------|------|
| 1 | レイテンシ | ワウ＆フラッターの遅延バッファにより、最大で約 30ms のレイテンシが発生する |
| 2 | CPU 負荷 | `tanhf` の呼び出し頻度が高い。負荷が問題になる場合は多項式近似（3次）に置換する |
| 3 | サンプルレート変更 | 曲間でサンプルレートが変わった場合、フィルター係数とバッファを再初期化する必要がある |
| 4 | チャンネル数 | モノラル〜ステレオを想定。5.1ch 等の場合もチャンネルごとに独立処理で動作するが、検証は未実施 |
| 5 | ワウの位相 | 左右チャンネルで同一の LFO 位相を使用する（実機の挙動に準拠） |

---

## 9. 将来の拡張案

- テープスピード切替（通常速 / 倍速 / ハーフスピード）でフィルター特性を自動調整
- テープタイプ選択（Type I / II / IV）によるサチュレーション特性の切替
- ステレオクロストーク（左右チャンネル間の微小な混入）の追加
- ドロップアウト（瞬間的な音切れ）のシミュレーション
- プリセットのインポート / エクスポート機能
