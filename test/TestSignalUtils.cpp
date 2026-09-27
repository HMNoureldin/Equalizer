/**
 * @file
 * @brief Implement test signal generation and measurement.
 */
#include "TestSignalUtils.hpp"

#include <cmath>

namespace testutils
{
namespace
{
/// Pi used to calculate sine-wave phase.
constexpr float kPi = 3.14159265358979323846f;
} // namespace

std::vector<float> generateSine(float frequencyHz, float amplitude,
                                std::size_t sampleCount)
{
    std::vector<float> samples(sampleCount);

    for (std::size_t n = 0; n < sampleCount; ++n) {
        const float phase = 2.0f * kPi * frequencyHz * static_cast<float>(n) /
                            equalizer_app::config::kSampleRateHz;

        samples[n] = amplitude * std::sin(phase);
    }

    return samples;
}

float calculateRms(const std::vector<float>& samples, std::size_t startIndex)
{
    double sumSquares = 0.0;

    for (std::size_t i = startIndex; i < samples.size(); ++i) {
        const float sample = samples[i];
        sumSquares += static_cast<double>(sample) * static_cast<double>(sample);
    }

    const double meanSquare = sumSquares / (samples.size() - startIndex);

    return static_cast<float>(std::sqrt(meanSquare));
}

float amplitudeRatioToDb(float ratio)
{
    return 20.0f * std::log10(ratio);
}

} // namespace testutils
