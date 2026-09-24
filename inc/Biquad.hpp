/**
 * @file
 * @author H.Noureldin <heshamnoureldin2017@gmail.com>
 * @brief Interface for a peaking biquad audio filter.
 */

#pragma once

/// Reusable audio equalizer processing components.
namespace audioeq
{
/// Peaking biquad filter.
class Biquad
{
  public:
    /// Initialize a pass-through filter.
    Biquad() = default;

    /**
     * @brief Validate and configure the band, keeping
     * state.
     * @param sampleRateHz Sample rate in Hz.
     * @param frequencyHz Center frequency in Hz.
     * @param q Quality factor (band width).
     * @param gainDb Boost or cut in dB.
     * @throws std::invalid_argument For nonfinite inputs,
     * nonpositive sample rate or Q, or frequency outside
     * (0, sampleRateHz / 2).
     * @note Invalid input leaves the filter unchanged.
     */
    void configurePeaking(float sampleRateHz, float frequencyHz, float q,
                          float gainDb);

    /**
     * @brief Process one sample and update filter state.
     * @param input Input audio sample.
     * @return Filtered audio sample.
     */
    float processSample(float input) noexcept;

    /// Clear sample history, keeping the coefficients.
    void reset() noexcept;

  private:
    /// Normalized numerator coefficient.
    float b0_{1.0f};
    /// Normalized numerator coefficient.
    float b1_{0.0f};
    /// Normalized numerator coefficient.
    float b2_{0.0f};
    /// Normalized denominator coefficient.
    float a1_{0.0f};
    /// Normalized denominator coefficient.
    float a2_{0.0f};

    /// First state value.
    float z1_{0.0f};
    /// Second state value.
    float z2_{0.0f};
};
} // namespace audioeq
