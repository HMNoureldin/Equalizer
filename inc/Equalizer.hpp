/**
 * @file
 * @author H.Noureldin <heshamnoureldin2017@gmail.com>
 * @brief Two-band audio equalizer interface.
 */

#pragma once

#include "Biquad.hpp"

#include <cstddef>

namespace audioeq
{
/**
 * @brief Two configurable peaking bands for one audio
 * channel.
 * @note Owns fixed-size filter state; no heap allocation is
 * needed. Configure before processing. Calls that modify
 * the same instance must not run concurrently with
 * process().
 */
class Equalizer
{
  public:
    /// Minimum accepted gain in dB.
    static constexpr float kMinGainDb = -12.0f;
    /// Maximum accepted gain in dB.
    static constexpr float kMaxGainDb = 12.0f;

    /// Initial band configuration; frequency and Q stay
    /// fixed.
    struct BandConfig
    {
        float frequencyHz;   ///< Center frequency in Hz.
        float q;             ///< Positive quality factor; higher is
                             ///< narrower.
        float gainDb = 0.0f; ///< Initial gain within the
                             ///< public gain limits.
    };

    /// Identify a band by its position in the processing
    /// chain.
    enum class Band {
        First, ///< First filter in the cascade.
        Second ///< Second filter in the cascade.
    };

    /**
     * @brief Configure two bands with their initial gains.
     * @param sampleRateHz Finite positive sample rate in
     * Hz.
     * @param firstBand First band's frequency, Q, and gain.
     * @param secondBand Second band's frequency, Q, and
     * gain.
     * @throws std::invalid_argument For nonfinite
     * parameters, nonpositive rate or Q, or frequencies
     * outside the open interval (0, sampleRateHz / 2),
     * or gains outside [kMinGainDb, kMaxGainDb].
     */
    Equalizer(float sampleRateHz, BandConfig firstBand, BandConfig secondBand);

    /**
     * @brief Change one band's absolute gain in decibels,
     * keeping sample history.
     * @param band Band to update.
     * @param gainDb Finite gain in [kMinGainDb,
     * kMaxGainDb]; replaces the previous gain rather than
     * adding to it.
     * @return True on success; false for an invalid band or
     * gain, leaving both filters unchanged.
     * @note Recalculates coefficients outside the
     * processing loop. Live gain smoothing and concurrent
     * updates are not supported.
     */
    [[nodiscard]] bool changeBandGainDb(Band band, float gainDb);

    /**
     * @brief Process one channel in place, keeping history.
     * @param samples Buffer of finite samples; null is a
     * no-op.
     * @param sampleCount Number of writable samples in the
     * buffer.
     * @note Output is not clipped. Use one instance per
     * channel. Processing is O(sampleCount) with fixed
     * memory, no allocation, I/O, locks, or extra block
     * buffering. Filter delay depends on the configured
     * frequencies and Q.
     */
    void process(float* samples, std::size_t sampleCount) noexcept;

    /// Clear sample history without changing gains.
    void reset() noexcept;

  private:
    /// Sample rate shared by both filters.
    float sampleRateHz_;

    /// Configuration retained for later gain updates.
    BandConfig firstBand_;
    /// Configuration retained for later gain updates.
    BandConfig secondBand_;

    /// First band in the processing chain.
    Biquad firstFilter_;
    /// Second band in the processing chain.
    Biquad secondFilter_;
};
} // namespace audioeq
