/**
 * @file
 * @author H.Noureldin <heshamnoureldin2017@gmail.com>
 * @brief Tests for the Equalizer application.
 */
#include "AudioConfig.hpp"
#include <audioeq/Equalizer.hpp>
#include "TestSignalUtils.hpp"

#include <algorithm>
#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <iostream>
#include <vector>

namespace
{

/**
 * @brief Check one band's steady-state gain with the other band at 0 dB.
 * @param frequencyHz Center frequency of the band under test (1000 or 2000).
 * @param gainDb Requested gain in decibels.
 * @note Measures identical input/output intervals after 0.1 seconds of
 * settling. The 0.1 dB tolerance is a project test criterion.
 */
void checkCenterFrequencyGain(float frequencyHz, float gainDb)
{
    constexpr std::size_t sampleCount = 48000;
    constexpr std::size_t blockSize = audioeq::kProcessingBlockSize;
    constexpr std::size_t settlingSamples = 4800;
    constexpr float toleranceDb = 0.1f;

    // Generate a test signal with a specific frequency and duration.
    const auto input = testutils::generateSine(frequencyHz, 0.1f, sampleCount);

    // Copy the input to the output.
    auto output = input;

    // Initialize the equalizer
    audioeq::Equalizer equalizer(
        testutils::kSampleRateHz,
        {1000.0f, audioeq::kSelectedQ, frequencyHz == 1000.0f ? gainDb : 0.0f},
        {2000.0f, audioeq::kSelectedQ, frequencyHz == 2000.0f ? gainDb : 0.0f});

    // Process the audio in blocks to simulate real-time processing.
    for (std::size_t offset = 0; offset < sampleCount; offset += blockSize) {
        equalizer.process(output.data() + offset,
                          std::min(blockSize, sampleCount - offset));
    }

    // Validate the output
    for (std::size_t i = 0; i < sampleCount; ++i) {
        ASSERT_TRUE(std::isfinite(output[i])) << "Sample index: " << i;
        ASSERT_LT(std::abs(output[i]), 1.0f)
            << "Test signal exceeds PCM headroom at sample " << i;
    }

    // Calculate RMS values for input and output
    const float inputRms = testutils::calculateRms(input, settlingSamples);
    const float outputRms = testutils::calculateRms(output, settlingSamples);

    // Both RMS values must be positive and finite.
    ASSERT_GT(inputRms, 0.0f);
    ASSERT_GT(outputRms, 0.0f);

    // Calculate the measured gain in decibels.
    const float measuredGainDb =
        testutils::amplitudeRatioToDb(outputRms / inputRms);

    // Log the results
    std::cout << "Tone: " << frequencyHz << " Hz; requested: " << gainDb
              << " dB; measured: " << measuredGainDb << " dB\n";

    EXPECT_NEAR(measuredGainDb, gainDb, toleranceDb);
}

} // namespace

/**
 * @brief Verify that both bands at 0 dB preserve every input sample.
 * @note Uses a quiet mixture of tones and a short final block. No initial
 * samples are discarded: unity gain must preserve the signal from startup.
 */
TEST(EqualizerTest, ZeroGainPreservesInput)
{
    constexpr std::size_t sampleCount = 48000;
    constexpr std::size_t blockSize = audioeq::kProcessingBlockSize;
    constexpr float toneAmplitude = 0.1f;
    constexpr float gainDb = 0.0f;
    constexpr float sampleTolerance = 1.0e-6f;
    constexpr float toneFrequenciesHz[] = {1000.0f, 2000.0f, 250.0f, 6000.0f};

    // Mix the two band centers with frequencies outside the bands.
    std::vector<float> input(sampleCount, 0.0f);
    for (const float frequencyHz : toneFrequenciesHz) {
        const auto tone =
            testutils::generateSine(frequencyHz, toneAmplitude, sampleCount);
        for (std::size_t sampleIndex = 0; sampleIndex < sampleCount;
             ++sampleIndex) {
            input[sampleIndex] += tone[sampleIndex];
        }
    }

    // Initialize the equalizer
    audioeq::Equalizer equalizer(testutils::kSampleRateHz,
                                 {1000.0f, audioeq::kSelectedQ, gainDb},
                                 {2000.0f, audioeq::kSelectedQ, gainDb});

    // Process a copy so the original remains available for comparison.
    auto output = input;
    for (std::size_t offset = 0; offset < sampleCount; offset += blockSize) {
        const std::size_t remainingSamples = sampleCount - offset;
        const std::size_t samplesInBlock =
            std::min(blockSize, remainingSamples);
        float* blockStart = output.data() + offset;

        equalizer.process(blockStart, samplesInBlock);
    }

    // Every output sample must remain finite and match the input and
    //  allow for a small tolerance.
    for (std::size_t sampleIndex = 0; sampleIndex < sampleCount;
         ++sampleIndex) {
        ASSERT_TRUE(std::isfinite(output[sampleIndex]))
            << "Sample index: " << sampleIndex;
        EXPECT_NEAR(output[sampleIndex], input[sampleIndex], sampleTolerance)
            << "Sample index: " << sampleIndex;
    }
}

// ========================================================================
/// Verify a +6 dB boost at the first band's center frequency.
TEST(EqualizerTest, Boost1kHzAppliesRequestedGain)
{
    checkCenterFrequencyGain(1000.0f, 6.0f);
}

/// Verify a -6 dB cut at the first band's center frequency.
TEST(EqualizerTest, Cut1kHzAppliesRequestedGain)
{
    checkCenterFrequencyGain(1000.0f, -6.0f);
}

/// Verify a +6 dB boost at the second band's center frequency.
TEST(EqualizerTest, Boost2kHzAppliesRequestedGain)
{
    checkCenterFrequencyGain(2000.0f, 6.0f);
}

/// Verify a -6 dB cut at the second band's center frequency.
TEST(EqualizerTest, Cut2kHzAppliesRequestedGain)
{
    checkCenterFrequencyGain(2000.0f, -6.0f);
}
