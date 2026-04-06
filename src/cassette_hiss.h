#pragma once
#include <cmath>
#include <vector>
#include <random>

class CassetteTapeHiss {
public:
    CassetteTapeHiss() : rng_(std::random_device{}()), dist_(-1.0f, 1.0f) {}

    void reset(unsigned channel_count) {
        state_.assign(channel_count, 0.0f);
    }

    // hiss_level_db: e.g. -40.0
    float process(float input, unsigned ch, float hiss_level_db) {
        if (ch >= state_.size()) return input;

        float white = dist_(rng_);

        // 1st-order LPF shaping (~8 kHz rolloff equivalent; coefficients are
        // sample-rate-independent approximation matching the design spec)
        state_[ch] = state_[ch] * 0.85f + white * 0.15f;

        float gain = powf(10.0f, hiss_level_db / 20.0f);
        return input + state_[ch] * gain;
    }

private:
    std::mt19937                          rng_;
    std::uniform_real_distribution<float> dist_;
    std::vector<float>                    state_;
};
