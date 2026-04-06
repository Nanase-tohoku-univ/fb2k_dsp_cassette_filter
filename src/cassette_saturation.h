#pragma once
#include <cmath>

class CassetteSaturation {
public:
    // Stateless: no init needed

    float process(float input, float drive, float mix) const {
        float driven     = input * drive;
        float saturated  = tanhf(driven);

        // Gain compensation so perceived volume stays constant as drive increases
        float compensation = 1.0f / tanhf(drive);
        saturated *= compensation;

        return input * (1.0f - mix) + saturated * mix;
    }
};
