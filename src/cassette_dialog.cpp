#include "pch.h"
#include "cassette_dialog.h"
#include "dsp_cassette.h"

// ---- preset I/O ----

void CassetteDialog::parse_preset(const dsp_preset & preset) {
    cfg_ = CassetteConfig{};
    if (preset.get_data_size() == 0) return;

    try {
        dsp_preset_parser parser(preset);
        uint32_t version = 0;
        parser >> version;
        if (version > CONFIG_VERSION) return;
        parser >> cfg_.lpf_cutoff_hz;
        parser >> cfg_.hiss_enabled;
        parser >> cfg_.hiss_level_db;
        parser >> cfg_.wf_rate_hz;
        parser >> cfg_.wf_depth;
        parser >> cfg_.wf_randomness;
        parser >> cfg_.sat_drive;
        parser >> cfg_.sat_mix;
    } catch (...) {
        cfg_ = CassetteConfig{};
    }
}

void CassetteDialog::build_preset(dsp_preset & out) const {
    dsp_preset_builder builder;
    builder << static_cast<uint32_t>(CONFIG_VERSION);
    builder << cfg_.lpf_cutoff_hz;
    builder << cfg_.hiss_enabled;
    builder << cfg_.hiss_level_db;
    builder << cfg_.wf_rate_hz;
    builder << cfg_.wf_depth;
    builder << cfg_.wf_randomness;
    builder << cfg_.sat_drive;
    builder << cfg_.sat_mix;
    builder.finish(g_cassette_guid, out);
}

// ---- Dialog handlers ----

LRESULT CassetteDialog::OnInitDialog(UINT, WPARAM, LPARAM, BOOL &) {
    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_BAR_CLASSES };
    InitCommonControlsEx(&icc);

    UpdateControls();
    return TRUE;
}

void CassetteDialog::UpdateControls() {
    // Low Pass
    SetupSlider(IDC_SLIDER_LPF_CUTOFF,
                FloatToSlider(cfg_.lpf_cutoff_hz, 2000.0f, 20000.0f));

    // Hiss
    ::CheckDlgButton(m_hWnd, IDC_CHECK_HISS_ENABLED,
                     cfg_.hiss_enabled ? BST_CHECKED : BST_UNCHECKED);
    SetupSlider(IDC_SLIDER_HISS_LEVEL,
                FloatToSlider(cfg_.hiss_level_db, -60.0f, -20.0f));

    // Wow & Flutter
    SetupSlider(IDC_SLIDER_WF_RATE,
                FloatToSlider(cfg_.wf_rate_hz, 0.5f, 15.0f));
    SetupSlider(IDC_SLIDER_WF_DEPTH,
                FloatToSlider(cfg_.wf_depth, 0.0f, 1.0f));
    SetupSlider(IDC_SLIDER_WF_RANDOM,
                FloatToSlider(cfg_.wf_randomness, 0.0f, 1.0f));

    // Saturation
    SetupSlider(IDC_SLIDER_SAT_DRIVE,
                FloatToSlider(cfg_.sat_drive, 1.0f, 10.0f));
    SetupSlider(IDC_SLIDER_SAT_MIX,
                FloatToSlider(cfg_.sat_mix, 0.0f, 1.0f));

    // Update static labels
    ApplyConfig();
}

void CassetteDialog::ApplyConfig() {
    // Read sliders into cfg_
    cfg_.lpf_cutoff_hz  = SliderToFloat(GetSliderPos(IDC_SLIDER_LPF_CUTOFF), 2000.0f, 20000.0f);
    cfg_.hiss_enabled   = (::IsDlgButtonChecked(m_hWnd, IDC_CHECK_HISS_ENABLED) == BST_CHECKED);
    cfg_.hiss_level_db  = SliderToFloat(GetSliderPos(IDC_SLIDER_HISS_LEVEL), -60.0f, -20.0f);
    cfg_.wf_rate_hz     = SliderToFloat(GetSliderPos(IDC_SLIDER_WF_RATE),    0.5f,   15.0f);
    cfg_.wf_depth       = SliderToFloat(GetSliderPos(IDC_SLIDER_WF_DEPTH),   0.0f,    1.0f);
    cfg_.wf_randomness  = SliderToFloat(GetSliderPos(IDC_SLIDER_WF_RANDOM),  0.0f,    1.0f);
    cfg_.sat_drive      = SliderToFloat(GetSliderPos(IDC_SLIDER_SAT_DRIVE),  1.0f,   10.0f);
    cfg_.sat_mix        = SliderToFloat(GetSliderPos(IDC_SLIDER_SAT_MIX),    0.0f,    1.0f);

    // Update labels (uSetDlgItemText handles UTF-8 -> Unicode conversion)
    pfc::string8 buf;

    buf.reset(); buf << pfc::format_int(static_cast<int>(cfg_.lpf_cutoff_hz)) << " Hz";
    uSetDlgItemText(m_hWnd, IDC_STATIC_LPF_CUTOFF, buf);

    buf.reset(); buf << pfc::format_float(cfg_.hiss_level_db, 0, 1) << " dB";
    uSetDlgItemText(m_hWnd, IDC_STATIC_HISS_LEVEL, buf);

    buf.reset(); buf << pfc::format_float(cfg_.wf_rate_hz, 0, 1) << " Hz";
    uSetDlgItemText(m_hWnd, IDC_STATIC_WF_RATE, buf);

    buf.reset(); buf << pfc::format_float(cfg_.wf_depth, 0, 2);
    uSetDlgItemText(m_hWnd, IDC_STATIC_WF_DEPTH, buf);

    buf.reset(); buf << pfc::format_float(cfg_.wf_randomness, 0, 2);
    uSetDlgItemText(m_hWnd, IDC_STATIC_WF_RANDOM, buf);

    buf.reset(); buf << pfc::format_float(cfg_.sat_drive, 0, 1) << "x";
    uSetDlgItemText(m_hWnd, IDC_STATIC_SAT_DRIVE, buf);

    buf.reset(); buf << pfc::format_int(static_cast<int>(cfg_.sat_mix * 100.0f)) << "%";
    uSetDlgItemText(m_hWnd, IDC_STATIC_SAT_MIX, buf);

    // Notify host for live preview
    dsp_preset_impl preset;
    build_preset(preset);
    callback_.on_preset_changed(preset);
}

LRESULT CassetteDialog::OnHScroll(UINT, WPARAM, LPARAM, BOOL &) {
    ApplyConfig();
    return 0;
}

LRESULT CassetteDialog::OnHissToggle(WORD, WORD, HWND, BOOL &) {
    ApplyConfig();
    return 0;
}

LRESULT CassetteDialog::OnReset(WORD, WORD, HWND, BOOL &) {
    cfg_ = CassetteConfig{};
    UpdateControls();
    return 0;
}

LRESULT CassetteDialog::OnOK(WORD, WORD, HWND, BOOL &) {
    ApplyConfig();
    ::EndDialog(m_hWnd, IDOK);
    return 0;
}

LRESULT CassetteDialog::OnCancel(WORD, WORD, HWND, BOOL &) {
    ::EndDialog(m_hWnd, IDCANCEL);
    return 0;
}
