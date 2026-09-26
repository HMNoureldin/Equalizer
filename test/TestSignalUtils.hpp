/**
 * @file
 * @brief Shared test signal generation and measurement helpers.
 */
#pragma once

#include <cstddef>
#include <vector>

/// Signal utilities for tests; independent of GoogleTest and the DSP core.
namespace testutils
{
/// Sample rate in Hz used by generated test signals.
inline constexpr float kSampleRateHz = 48000.0f;

/**
 * @brief Generate a sine wave at the test sample rate, starting at zero phase.
 * @param frequencyHz Tone frequency in Hz.
 * @param amplitude Peak amplitude of the generated samples.
 * @param sampleCount Number of samples to generate.
 * @return Generated samples; empty when sampleCount is zero.
 */
std::vector<float> generateSine(float frequencyHz, float amplitude,
                                std::size_t sampleCount);

/**
 * @brief Measure the root-mean-square level after a settling interval.
 * @param samples Nonempty buffer of finite audio samples.
 * @param startIndex First sample included in the measurement.
 * @return Root-mean-square amplitude of the remaining samples.
 * @pre startIndex must be less than samples.size().
 */
float calculateRms(const std::vector<float>& samples, std::size_t startIndex);

/**
 * @brief Convert an amplitude ratio to decibels.
 * @param ratio Positive finite output-to-input amplitude ratio.
 * @return Gain in decibels, calculated as 20 * log10(ratio).
 * @pre ratio must be positive and finite.
 */
float amplitudeRatioToDb(float ratio);

} // namespace testutils
