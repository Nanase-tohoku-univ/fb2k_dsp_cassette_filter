#pragma once
#include "pch.h"
#include "res/resource.h"
#include "cassette_config.h"

// Forward declaration
class foo_dsp_cassette;

class CassetteDialog : public CDialogImpl<CassetteDialog> {
public:
    enum { IDD = IDD_CASSETTE_CONFIG };

    CassetteDialog(const dsp_preset & preset, dsp_preset_edit_callback & callback)
        : callback_(callback)
    {
        parse_preset(preset);
    }

    BEGIN_MSG_MAP(CassetteDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_HSCROLL,    OnHScroll)
        COMMAND_HANDLER(IDC_CHECK_HISS_ENABLED, BN_CLICKED, OnHissToggle)
        COMMAND_ID_HANDLER(IDC_BTN_RESET, OnReset)
        COMMAND_ID_HANDLER(IDOK,           OnOK)
        COMMAND_ID_HANDLER(IDCANCEL,       OnCancel)
    END_MSG_MAP()

private:
    dsp_preset_edit_callback & callback_;
    CassetteConfig              cfg_;

    // Slider helpers – all sliders use integer range [0, 1000]
    static constexpr int SLIDER_RANGE = 1000;

    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL &);
    LRESULT OnHScroll(UINT, WPARAM, LPARAM, BOOL &);
    LRESULT OnHissToggle(WORD, WORD, HWND, BOOL &);
    LRESULT OnReset(WORD, WORD, HWND, BOOL &);
    LRESULT OnOK(WORD, WORD, HWND, BOOL &);
    LRESULT OnCancel(WORD, WORD, HWND, BOOL &);

    // Populate dialog controls from cfg_
    void UpdateControls();

    // Read all controls into cfg_ and notify host
    void ApplyConfig();

    void SetupSlider(int id, int pos) {
        CTrackBarCtrl tb(::GetDlgItem(m_hWnd, id));
        tb.SetRange(0, SLIDER_RANGE, FALSE);
        tb.SetPos(pos);
    }

    int GetSliderPos(int id) {
        CTrackBarCtrl tb(::GetDlgItem(m_hWnd, id));
        return tb.GetPos();
    }

    // Mapping helpers
    static int   FloatToSlider(float v, float lo, float hi) {
        return static_cast<int>((v - lo) / (hi - lo) * SLIDER_RANGE + 0.5f);
    }
    static float SliderToFloat(int pos, float lo, float hi) {
        return lo + static_cast<float>(pos) / SLIDER_RANGE * (hi - lo);
    }

    void parse_preset(const dsp_preset & preset);
    void build_preset(dsp_preset & out) const;
};
