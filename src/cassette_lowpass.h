#pragma once
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class CassetteLowPass {
public:
    void update_coefficients(unsigned sample_rate, float cutoff_hz) {
        const float Q = 0.7071f; // Butterworth
        float omega     = 2.0f * static_cast<float>(M_PI) * cutoff_hz / static_cast<float>(sample_rate);
        float sin_omega = sinf(omega);
        float cos_omega = cosf(omega);
        float alpha     = sin_omega / (2.0f * Q);

        float a0_inv = 1.0f / (1.0f + alpha);
        b0_ = ((1.0f - cos_omega) / 2.0f) * a0_inv;
        b1_ = (1.0f - cos_omega) * a0_inv;
        b2_ = b0_;
        a1_ = (-2.0f * cos_omega) * a0_inv;
        a2_ = (1.0f - alpha) * a0_inv;
    }

    void reset(unsigned channel_count) {
        x1_.assign(channel_count, 0.0f);
        x2_.assign(channel_count, 0.0f);
        y1_.assign(channel_count, 0.0f);
        y2_.assign(channel_count, 0.0f);
    }

    float process(float input, unsigned ch) {
        if (ch >= x1_.size()) return input;

        float y = b0_ * input + b1_ * x1_[ch] + b2_ * x2_[ch]
                              - a1_ * y1_[ch]  - a2_ * y2_[ch];

        x2_[ch] = x1_[ch];
        x1_[ch] = input;
        y2_[ch] = y1_[ch];
        y1_[ch] = y;

        return y;
    }

private:
    float b0_ = 1.0f, b1_ = 0.0f, b2_ = 0.0f;
    float a1_ = 0.0f, a2_ = 0.0f;

    std::vector<float> x1_, x2_, y1_, y2_;
};
