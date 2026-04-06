#include "pch.h"
#include "dsp_cassette.h"
#include "cassette_dialog.h"

DECLARE_COMPONENT_VERSION(
	"Cassette Tape Emulator DSP",
	"0.1",
	""
);

// ---- Serialization ----

void foo_dsp_cassette::serialize(const CassetteConfig & c, dsp_preset & out) {
    dsp_preset_builder builder;
    builder << static_cast<uint32_t>(CONFIG_VERSION);
    builder << c.lpf_cutoff_hz;
    builder << c.hiss_enabled;
    builder << c.hiss_level_db;
    builder << c.wf_rate_hz;
    builder << c.wf_depth;
    builder << c.wf_randomness;
    builder << c.sat_drive;
    builder << c.sat_mix;
    builder.finish(g_get_guid(), out);
}

bool foo_dsp_cassette::parse_config(const dsp_preset & in, CassetteConfig & out) {
    try {
        dsp_preset_parser parser(in);
        uint32_t version = 0;
        parser >> version;
        if (version > CONFIG_VERSION) return false;

        parser >> out.lpf_cutoff_hz;
        parser >> out.hiss_enabled;
        parser >> out.hiss_level_db;
        parser >> out.wf_rate_hz;
        parser >> out.wf_depth;
        parser >> out.wf_randomness;
        parser >> out.sat_drive;
        parser >> out.sat_mix;
        return true;
    } catch (...) {
        out = CassetteConfig{}; // revert to defaults on parse error
        return false;
    }
}

// ---- Main processing ----

bool foo_dsp_cassette::on_chunk(audio_chunk * chunk, abort_callback &) {
    unsigned sr = chunk->get_sample_rate();
    unsigned ch = chunk->get_channel_count();

    if (sr != current_sample_rate_ || ch != current_channels_) {
        reinit(sr, ch);
    }

    audio_sample * data    = chunk->get_data();
    t_size         samples = chunk->get_sample_count();

    for (t_size i = 0; i < samples; ++i) {
        for (unsigned c = 0; c < ch; ++c) {
            t_size idx    = i * ch + c;
            float  sample = static_cast<float>(data[idx]);

            // ① Saturation
            sample = saturation_.process(sample, cfg_.sat_drive, cfg_.sat_mix);

            // ② Low-pass filter
            sample = lowpass_.process(sample, c);

            // ③ Wow & Flutter
            sample = wow_flutter_.process(sample, c,
                                          cfg_.wf_rate_hz,
                                          cfg_.wf_depth,
                                          cfg_.wf_randomness);

            // ④ Tape hiss
            if (cfg_.hiss_enabled) {
                sample = hiss_.process(sample, c, cfg_.hiss_level_db);
            }

            data[idx] = static_cast<audio_sample>(sample);
        }

        // Advance LFO phase once per sample (all channels share same LFO)
        wow_flutter_.advance_phase(cfg_.wf_rate_hz, sr);
    }

    return true;
}

// ---- Default preset ----

bool foo_dsp_cassette::g_get_default_preset(dsp_preset & p_out) {
    CassetteConfig defaults;
    serialize(defaults, p_out);
    return true;
}

// ---- Config popup ----

void foo_dsp_cassette::g_show_config_popup(const dsp_preset & p_data,
                                            HWND p_parent,
                                            dsp_preset_edit_callback & p_callback) {
    CassetteDialog dlg(p_data, p_callback);
    dlg.DoModal(p_parent);
}

// ---- DSP factory registration ----

static dsp_factory_t<foo_dsp_cassette, dsp_entry_v2> g_cassette_factory;
