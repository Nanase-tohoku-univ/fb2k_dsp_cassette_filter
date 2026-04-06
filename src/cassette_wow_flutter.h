#pragma once
#include <cmath>
#include <vector>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class CassetteWowFlutter {
public:
    CassetteWowFlutter() : rng_(std::random_device{}()), dist_(-1.0f, 1.0f) {}

    void update_sample_rate(unsigned sample_rate) {
        sample_rate_ = sample_rate;
        // max delay ~30 ms or 3% of sample rate (whichever is smaller for safety)
        max_delay_samples_ = static_cast<int>(sample_rate * 0.03f);
        if (max_delay_samples_ < 8) max_delay_samples_ = 8;
    }

    void reset(unsigned channel_count) {
        channels_ = channel_count;
        int buf_size = max_delay_samples_ * 2;

        delay_buffer_.assign(channel_count, std::vector<float>(buf_size, 0.0f));
        write_pos_.assign(channel_count, 0);
        noise_state_.assign(channel_count, 0.0f);

        phase_ = 0.0f;
    }

    float process(float input, unsigned ch,
                  float rate_hz, float depth, float randomness) {
        if (ch >= channels_) return input;

        int   buf_size = static_cast<int>(delay_buffer_[ch].size());
        int   wp       = write_pos_[ch];

        // Write input into ring buffer
        delay_buffer_[ch][wp] = input;

        // LFO component
        float lfo = sinf(2.0f * static_cast<float>(M_PI) * phase_);

        // Random noise component (low-passed)
        float raw_noise   = dist_(rng_);
        noise_state_[ch]  = noise_state_[ch] * 0.98f + raw_noise * 0.02f;
        float noise       = noise_state_[ch];

        float mod   = lfo * (1.0f - randomness) + noise * randomness;
        float base  = static_cast<float>(max_delay_samples_) / 2.0f;
        float delay = base + mod * depth * (static_cast<float>(max_delay_samples_) / 2.0f);
        if (delay < 1.0f)  delay = 1.0f;
        if (delay > static_cast<float>(max_delay_samples_ - 1)) {
            delay = static_cast<float>(max_delay_samples_ - 1);
        }

        // Fractional delay read with linear interpolation
        float read_pos_f = static_cast<float>(wp) - delay;
        int   idx0       = static_cast<int>(floorf(read_pos_f));
        float frac       = read_pos_f - static_cast<float>(idx0);

        auto wrap = [&](int i) { return ((i % buf_size) + buf_size) % buf_size; };
        float out = delay_buffer_[ch][wrap(idx0)]     * (1.0f - frac)
                  + delay_buffer_[ch][wrap(idx0 + 1)] * frac;

        // Advance write pointer
        write_pos_[ch] = (wp + 1) % buf_size;

        return out;
    }

    // Call once per sample (after all channels processed)
    void advance_phase(float rate_hz, unsigned sample_rate) {
        phase_ += rate_hz / static_cast<float>(sample_rate);
        if (phase_ >= 1.0f) phase_ -= 1.0f;
    }

private:
    unsigned sample_rate_       = 44100;
    int      max_delay_samples_ = 1323; // ~30 ms at 44100
    unsigned channels_          = 0;

    std::vector<std::vector<float>> delay_buffer_;
    std::vector<int>                write_pos_;
    std::vector<float>              noise_state_;

    float phase_ = 0.0f;

    std::mt19937                          rng_;
    std::uniform_real_distribution<float> dist_;
};
