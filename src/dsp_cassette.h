#pragma once
#include "pch.h"
#include "cassette_config.h"
#include "cassette_saturation.h"
#include "cassette_lowpass.h"
#include "cassette_hiss.h"
#include "cassette_wow_flutter.h"

// {A3B4C5D6-E7F8-4901-A2B3-C4D5E6F70001}
// Replace with your own GUID before release
static const GUID g_cassette_guid = {
    0xa3b4c5d6, 0xe7f8, 0x4901,
    {0xa2, 0xb3, 0xc4, 0xd5, 0xe6, 0xf7, 0x00, 0x01}
};

class foo_dsp_cassette : public dsp_impl_base {
public:
    foo_dsp_cassette() = default;
    explicit foo_dsp_cassette(const dsp_preset & preset) {
        parse_config(preset, cfg_);
    }

    static GUID g_get_guid()             { return g_cassette_guid; }
    static void g_get_name(pfc::string_base & out) { out = "Cassette Tape Emulator"; }

    // Required by dsp_factory_t<> with dsp_entry_v2
    static bool g_get_default_preset(dsp_preset & p_out);
    static bool g_have_config_popup()   { return true; }
    static void g_show_config_popup(const dsp_preset & p_data,
                                    HWND p_parent,
                                    dsp_preset_edit_callback & p_callback);

    // ---- dsp_impl_base overrides ----
    void on_endoftrack(abort_callback &) override   { flush_state(); }
    void on_endofplayback(abort_callback &) override { flush_state(); }

    bool on_chunk(audio_chunk * chunk, abort_callback &) override;

    void flush() override { flush_state(); }

    double get_latency() override {
        if (current_sample_rate_ == 0) return 0.0;
        return 0.015; // ~15 ms conservative estimate
    }

    bool need_track_change_mark() override { return false; }

private:
    CassetteConfig     cfg_;
    CassetteSaturation saturation_;
    CassetteLowPass    lowpass_;
    CassetteTapeHiss   hiss_;
    CassetteWowFlutter wow_flutter_;

    unsigned current_sample_rate_ = 0;
    unsigned current_channels_    = 0;

    void flush_state() {
        lowpass_.reset(current_channels_);
        hiss_.reset(current_channels_);
        wow_flutter_.reset(current_channels_);
    }

    void reinit(unsigned sr, unsigned ch) {
        current_sample_rate_ = sr;
        current_channels_    = ch;
        lowpass_.update_coefficients(sr, cfg_.lpf_cutoff_hz);
        lowpass_.reset(ch);
        hiss_.reset(ch);
        wow_flutter_.update_sample_rate(sr);
        wow_flutter_.reset(ch);
    }

    static void serialize(const CassetteConfig & c, dsp_preset & out);
    static bool parse_config(const dsp_preset & in, CassetteConfig & out);

    friend class CassetteDialog;
};
