#pragma once
#include <cstdint>

static constexpr uint32_t CONFIG_VERSION = 1;

struct CassetteConfig {
    // Low Pass Filter
    float lpf_cutoff_hz  = 12000.0f;  // 2000 – 20000

    // Tape Hiss
    bool  hiss_enabled   = true;
    float hiss_level_db  = -40.0f;    // -60 – -20

    // Wow & Flutter
    float wf_rate_hz     = 3.0f;      // 0.5 – 15.0
    float wf_depth       = 0.3f;      // 0.0 – 1.0
    float wf_randomness  = 0.5f;      // 0.0 – 1.0

    // Saturation
    float sat_drive      = 2.0f;      // 1.0 – 10.0
    float sat_mix        = 0.5f;      // 0.0 – 1.0
};
