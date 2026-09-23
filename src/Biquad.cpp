/**
 * @file
 * @author H.Noureldin <heshamnoureldin2017@gmail.com>
 * @brief Peaking EQ with normalized biquad coefficients.
 * Coefficients follow Robert Bristow-Johnson's
 * Audio EQ Cookbook (peakingEQ).
 * @see
 * https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html
 */

#include "Biquad.hpp"

#include <cmath>
#include <stdexcept>

namespace
{
/// Pi for angular-frequency calculation
constexpr float kPi = 3.14159265358979323846f;

/**
 * @brief Check configuration before changing the filter.
 * @param sampleRateHz Sample rate in Hz; must be positive.
 * @param frequencyHz Frequency in (0, sampleRateHz / 2).
 * @param q Positive quality factor.
 * @param gainDb Finite gain in dB.
 * @throws std::invalid_argument For nonfinite values or
 * values outside the ranges above.
 */
void validateParameters(float sampleRateHz,
                        float frequencyHz, float q,
                        float gainDb)
{
    if (!std::isfinite(sampleRateHz) ||
        !std::isfinite(frequencyHz) || !std::isfinite(q) ||
        !std::isfinite(gainDb)) {
        throw std::invalid_argument(
            "Filter parameters must be finite.");
    }
    if (sampleRateHz <= 0.0f) {
        throw std::invalid_argument(
            "Sample rate must be positive.");
    }
    if (q <= 0.0f) {
        throw std::invalid_argument("Q must be positive.");
    }
    if (frequencyHz <= 0.0f ||
        frequencyHz >= sampleRateHz / 2.0f) {
        throw std::invalid_argument(
            "Frequency must be above 0 and below "
            "half the sample rate (Nyquist frequency).");
    }
}
} // namespace

void Biquad::configurePeaking(float sampleRateHz,
                              float frequencyHz, float q,
                              float gainDb)
{
    validateParameters(sampleRateHz, frequencyHz, q,
                       gainDb);

    // Compute intermediate values.
    const float A = std::pow(10.0f, gainDb / 40.0f);

    // calculate the normalized angular frequency
    const float omega =
        2.0f * kPi * frequencyHz / sampleRateHz;

    // calculate the alpha value for the filter
    const float alpha = std::sin(omega) / (2.0f * q);

    // calculate the cosine of the normalized angular
    // frequency
    const float cosOmega = std::cos(omega);

    // Peaking EQ coefficients before normalization.
    const float b0 = 1.0f + alpha * A;
    const float b1 = -2.0f * cosOmega;
    const float b2 = 1.0f - alpha * A;

    const float a0 = 1.0f + alpha / A;
    const float a1 = -2.0f * cosOmega;
    const float a2 = 1.0f - alpha / A;

    // Normalize everything by a0.
    b0_ = b0 / a0;
    b1_ = b1 / a0;
    b2_ = b2 / a0;

    a1_ = a1 / a0;
    a2_ = a2 / a0;
}

float Biquad::processSample(float input) noexcept
{
    // calculate the output sample
    const float output = b0_ * input + z1_;
    // update the filter state z1
    z1_ = b1_ * input - a1_ * output + z2_;
    // update the filter state z2
    z2_ = b2_ * input - a2_ * output;

    return output;
}

void Biquad::reset() noexcept
{
    z1_ = 0.0f;
    z2_ = 0.0f;
}
