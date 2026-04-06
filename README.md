# foo_dsp_cassette — Cassette Tape Emulator DSP for foobar2000

A foobar2000 DSP component that applies real-time cassette tape emulation effects to audio playback.

## Description

`foo_dsp_cassette` simulates the acoustic characteristics of cassette tape playback through a 4-stage processing chain:

```
Input PCM
  │
  ├─ ① Saturation      — tape magnetic saturation (tanh soft-clipping)
  ├─ ② Low-Pass Filter — high-frequency rolloff (2nd-order Butterworth IIR)
  ├─ ③ Wow & Flutter   — pitch modulation from motor speed variation
  └─ ④ Tape Hiss       — background noise

Output PCM
```

## Features

- **Tape Saturation** — tanh-based soft-clipping with harmonic distortion and gain compensation
- **High-Frequency Rolloff** — 2nd-order Butterworth IIR low-pass filter, adjustable cutoff
- **Wow & Flutter** — variable-delay line pitch modulation with sine LFO + filtered noise blend
- **Tape Hiss** — bandlimited white noise, switchable and level-adjustable
- Real-time parameter adjustment via foobar2000 DSP Manager UI
- Supports Win32 (x86) and x64

## Installation

1. Download the latest `foo_dsp_cassette.fb2k-component` from the [Releases](../../releases) page.
2. Drag and drop the file onto the foobar2000 window, or copy it to your foobar2000 `components` folder.
3. Restart foobar2000.
4. Open **File → Preferences → Playback → DSP Manager**.
5. Add **Cassette Tape Emulator** to the active DSP chain.
6. Click **Configure** to open the settings dialog.

## Parameters

### Low-Pass Filter

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Cutoff | 2000 – 20000 Hz | 12000 Hz | -3 dB point. Lower values produce a muddier, more muffled sound. |

### Tape Hiss

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Enable | on / off | on | Toggles background hiss noise. |
| Level | -60 – -20 dB | -40 dB | Amplitude of the hiss noise. |

### Wow & Flutter

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Rate | 0.5 – 15.0 Hz | 3.0 Hz | LFO frequency of pitch modulation. Low values (< 2 Hz) produce wow; high values (> 6 Hz) produce flutter. |
| Depth | 0.0 – 1.0 | 0.3 | Modulation depth. Controls the amount of pitch deviation. |
| Randomness | 0.0 – 1.0 | 0.5 | Blend between periodic sine LFO (0.0) and random noise modulation (1.0). |

### Saturation

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Drive | 1.0 – 10.0 | 2.0 | Input gain multiplier before the tanh clipper. Higher values add more harmonic distortion. |
| Mix | 0.0 – 1.0 | 0.5 | Dry/wet blend between the original and saturated signal. |

## Preset Suggestions

| Preset | Cutoff | Hiss | Hiss Level | Rate | Depth | Randomness | Drive | Mix |
|--------|--------|------|------------|------|-------|------------|-------|-----|
| High Quality Deck | 16000 Hz | on | -50 dB | 1.0 Hz | 0.1 | 0.2 | 1.5 | 0.3 |
| Standard Cassette | 12000 Hz | on | -40 dB | 3.0 Hz | 0.3 | 0.5 | 2.0 | 0.5 |
| Worn Out Tape | 8000 Hz | on | -30 dB | 5.0 Hz | 0.6 | 0.8 | 3.5 | 0.7 |
| Lo-Fi Extreme | 4000 Hz | on | -25 dB | 8.0 Hz | 0.9 | 0.9 | 6.0 | 0.9 |

## Building from Source

### Prerequisites

| Tool | Version |
|------|---------|
| Visual Studio | 2022 (v143 toolset) |
| foobar2000 SDK | Latest (from [foobar2000.org](https://www.foobar2000.org/SDK)) |
| WTL | 10.x (via NuGet, restored automatically) |
| C++ standard | C++17 |

### Steps

1. Clone this repository.
2. Place the foobar2000 SDK files under `lib/` as follows:
   ```
   lib/
   ├── foobar2000/
   ├── pfc/
   └── libPPUI/
   ```
3. Open `foo_dsp_cassette.sln` in Visual Studio 2022.
4. NuGet will restore WTL automatically on first build.
5. Build the **Release x64** configuration.
6. Rename the output `foo_dsp_cassette.dll` to `foo_dsp_cassette.fb2k-component` and install it.

## Known Limitations

- **Latency** — the wow & flutter delay buffer introduces up to ~30 ms of latency.
- **CPU cost** — `tanhf()` is called per sample; a polynomial approximation can be substituted if needed.
- **Multichannel** — tested with mono and stereo. 5.1ch and above are processed correctly by design but have not been verified in practice.
- **Shared LFO** — left and right channels share the same LFO phase for wow & flutter, matching real-world cassette deck behavior.

## License

This software is distributed under the **GNU General Public License v2.0**.
See [LICENSE](LICENSE) for details.

---

## 日本語説明

### 概要

`foo_dsp_cassette` は、foobar2000 のリアルタイム再生にカセットテープ特有の音響特性を付加する DSP コンポーネントです。

以下の 4 段階の処理を直列に適用します：

1. **サチュレーション** — テープ磁気飽和による倍音付加・ソフトクリッピング
2. **ローパスフィルター** — テープの高域減衰を 2 次 Butterworth IIR フィルターで再現
3. **ワウ＆フラッター** — モーター回転ムラに起因するピッチ揺れ
4. **テープヒス** — 「サー」というバックグラウンドノイズの付加

### インストール

1. [Releases](../../releases) ページから最新の `foo_dsp_cassette.fb2k-component` をダウンロードします。
2. ファイルを foobar2000 のウィンドウにドラッグ＆ドロップするか、`components` フォルダーにコピーします。
3. foobar2000 を再起動します。
4. **ファイル → 環境設定 → 再生 → DSP Manager** を開きます。
5. **Cassette Tape Emulator** をアクティブな DSP チェーンに追加します。
6. **Configure** ボタンをクリックして設定ダイアログを開きます。

### パラメータ

#### ローパスフィルター

| パラメータ | 範囲 | デフォルト | 説明 |
|-----------|------|-----------|------|
| Cutoff | 2000 – 20000 Hz | 12000 Hz | カットオフ周波数。下げるほどこもった音になります。 |

#### テープヒス

| パラメータ | 範囲 | デフォルト | 説明 |
|-----------|------|-----------|------|
| Enable | on / off | on | ヒスノイズの有効・無効を切り替えます。 |
| Level | -60 – -20 dB | -40 dB | ノイズの音量。 |

#### ワウ＆フラッター

| パラメータ | 範囲 | デフォルト | 説明 |
|-----------|------|-----------|------|
| Rate | 0.5 – 15.0 Hz | 3.0 Hz | 揺れの基本周波数。2 Hz 以下でワウ的、6 Hz 以上でフラッター的な揺れになります。 |
| Depth | 0.0 – 1.0 | 0.3 | 揺れの深さ（最大ピッチ偏差の割合）。 |
| Randomness | 0.0 – 1.0 | 0.5 | 0.0 で正弦波 LFO、1.0 でランダムノイズ変調になります。 |

#### サチュレーション

| パラメータ | 範囲 | デフォルト | 説明 |
|-----------|------|-----------|------|
| Drive | 1.0 – 10.0 | 2.0 | tanh クリッパー前の入力ゲイン倍率。大きいほど歪みが増します。 |
| Mix | 0.0 – 1.0 | 0.5 | ドライ/ウェット比。0.0 で原音のみ、1.0 で完全サチュレーション。 |

### ビルド

詳細な設計仕様は [`foo_dsp_cassette_design.md`](foo_dsp_cassette_design.md) を参照してください。

ビルドには Visual Studio 2022、foobar2000 SDK（最新版）、WTL 10.x が必要です。
SDK ファイルを `lib/` 以下に配置し、`foo_dsp_cassette.sln` を開いて Release x64 でビルドしてください。

### ライセンス

GNU General Public License v2.0 のもとで配布します。
