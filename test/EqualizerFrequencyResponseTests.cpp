/**
 * @file
 * @brief Frequency-response and cross-band isolation tests.
 */

#include "AudioConfig.hpp"
#include <audioeq/Equalizer.hpp>
#include "TestSignalUtils.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include <gtest/gtest.h>

namespace
{

/**
 * @brief Measure the unwanted effect of one EQ band on the other band's center.
 *
 * @param q Quality factor shared by both bands.
 * @param toneHz Center frequency that should remain unchanged (1000 or 2000
 * Hz).
 * @param otherBandGainDb Gain applied to the other band in decibels.
 *
 * @return Measured change of the unchanged tone in decibels.
 *
 * @note The first 0.1 seconds are ignored to allow the filters to settle.
 */
float measureBandLeakage(float q, float toneHz, float otherBandGainDb)
{
    constexpr std::size_t sampleCount = 48000;
    constexpr std::size_t blockSize = audioeq::kProcessingBlockSize;
    constexpr std::size_t settlingSamples = 4800;

    // Generate the tone that should remain unchanged.
    const auto input = testutils::generateSine(toneHz, 0.1f, sampleCount);

    // Keep the original input and process a copy.
    auto output = input;

    // Start with both bands at 0 dB.
    float gain1000Hz = 0.0f;
    float gain2000Hz = 0.0f;

    // Apply gain only to the other band.
    if (toneHz == 1000.0f) {
        gain2000Hz = otherBandGainDb;
    } else {
        gain1000Hz = otherBandGainDb;
    }

    audioeq::Equalizer equalizer(testutils::kSampleRateHz,
                                 {1000.0f, q, gain1000Hz},
                                 {2000.0f, q, gain2000Hz});

    // Process the audio in blocks.
    for (std::size_t offset = 0; offset < sampleCount; offset += blockSize) {

        equalizer.process(output.data() + offset,
                          std::min(blockSize, sampleCount - offset));
    }

    // Measure the signal level after the filter has settled.
    const float inputRms = testutils::calculateRms(input, settlingSamples);

    const float outputRms = testutils::calculateRms(output, settlingSamples);

    // Return how much the tone changed.
    return testutils::amplitudeRatioToDb(outputRms / inputRms);
}

/**
 * @brief Measure how much one EQ band changes another frequency.
 *
 * @param toneHz Frequency of the test tone.
 * @param centerHz EQ band to change (1000 or 2000 Hz).
 * @param gainDb Gain applied to the selected band.
 *
 * @return Measured change of the test tone in decibels.
 *
 * @note The first 0.1 seconds are ignored to allow the filters to settle.
 */
float measureFrequencyChange(float toneHz, float centerHz, float gainDb)
{
    constexpr std::size_t sampleCount = 48000;
    constexpr std::size_t blockSize = audioeq::kProcessingBlockSize;
    constexpr std::size_t settlingSamples = 4800;

    // Generate the frequency that we want to measure.
    const auto input = testutils::generateSine(toneHz, 0.1f, sampleCount);

    // Keep the original input and process a copy.
    auto output = input;

    // Start with both bands at 0 dB.
    float gain1000Hz = 0.0f;
    float gain2000Hz = 0.0f;

    // Apply gain only to the selected band.
    if (centerHz == 1000.0f) {
        gain1000Hz = gainDb;
    } else {
        gain2000Hz = gainDb;
    }

    audioeq::Equalizer equalizer(testutils::kSampleRateHz,
                                 {1000.0f, audioeq::kSelectedQ, gain1000Hz},
                                 {2000.0f, audioeq::kSelectedQ, gain2000Hz});

    // Process the audio in blocks.
    for (std::size_t offset = 0; offset < sampleCount; offset += blockSize) {

        equalizer.process(output.data() + offset,
                          std::min(blockSize, sampleCount - offset));
    }

    // Measure the signal level after the filter has settled.
    const float inputRms = testutils::calculateRms(input, settlingSamples);

    const float outputRms = testutils::calculateRms(output, settlingSamples);

    // Return how much the tone changed.
    return testutils::amplitudeRatioToDb(outputRms / inputRms);
}

} // namespace

// -----------------------------------------------------------------------------
// Band isolation tests
// -----------------------------------------------------------------------------

/// Boosting the 2 kHz band must change the 1 kHz band by less than 0.5 dB.
TEST(EqualizerIsolation, Boost2kHzDoesNotAffect1kHz)
{
    constexpr float isolationLimitDb = 0.5f;

    const float leakageDb =
        measureBandLeakage(audioeq::kSelectedQ, 1000.0f, 12.0f);

    EXPECT_LT(std::abs(leakageDb), isolationLimitDb);
}

/// Cutting the 2 kHz band must change the 1 kHz band by less than 0.5 dB.
TEST(EqualizerIsolation, Cut2kHzDoesNotAffect1kHz)
{
    constexpr float isolationLimitDb = 0.5f;

    const float leakageDb =
        measureBandLeakage(audioeq::kSelectedQ, 1000.0f, -12.0f);

    EXPECT_LT(std::abs(leakageDb), isolationLimitDb);
}

/// Boosting the 1 kHz band must change the 2 kHz band by less than 0.5 dB.
TEST(EqualizerIsolation, Boost1kHzDoesNotAffect2kHz)
{
    constexpr float isolationLimitDb = 0.5f;

    const float leakageDb =
        measureBandLeakage(audioeq::kSelectedQ, 2000.0f, 12.0f);

    EXPECT_LT(std::abs(leakageDb), isolationLimitDb);
}

/// Cutting the 1 kHz band must change the 2 kHz band by less than 0.5 dB.
TEST(EqualizerIsolation, Cut1kHzDoesNotAffect2kHz)
{
    constexpr float isolationLimitDb = 0.5f;

    const float leakageDb =
        measureBandLeakage(audioeq::kSelectedQ, 2000.0f, -12.0f);

    EXPECT_LT(std::abs(leakageDb), isolationLimitDb);
}

// -----------------------------------------------------------------------------
// Neighbor-frequency tests
// -----------------------------------------------------------------------------

/**
 * @brief Verify that changing the 1 kHz band has little effect on
 *        frequencies far away from it.
 *
 * Each frequency is tested with the maximum boost and maximum cut.
 */
TEST(EqualizerNeighborResponse, OneKHzBandDoesNotAffectDistantFrequencies)
{
    constexpr float maximumChangeDb = 0.5f;

    // Frequencies away from the 1 kHz band.
    const float frequencies[] = {250.0f, 500.0f, 3000.0f, 4000.0f, 6000.0f};

    for (const float frequencyHz : frequencies) {

        // Test a +12 dB boost at 1 kHz.
        const float boostChangeDb =
            measureFrequencyChange(frequencyHz, 1000.0f, 12.0f);

        EXPECT_LT(std::abs(boostChangeDb), maximumChangeDb)
            << "Frequency: " << frequencyHz << " Hz";

        // Test a -12 dB cut at 1 kHz.
        const float cutChangeDb =
            measureFrequencyChange(frequencyHz, 1000.0f, -12.0f);

        EXPECT_LT(std::abs(cutChangeDb), maximumChangeDb)
            << "Frequency: " << frequencyHz << " Hz";
    }
}

/**
 * @brief Verify that changing the 2 kHz band has little effect on
 *        frequencies far away from it.
 *
 * Each frequency is tested with the maximum boost and maximum cut.
 */
TEST(EqualizerNeighborResponse, TwoKHzBandDoesNotAffectDistantFrequencies)
{
    constexpr float maximumChangeDb = 0.5f;

    // Frequencies away from the 2 kHz band.
    const float frequencies[] = {250.0f, 500.0f, 4000.0f, 6000.0f};

    for (const float frequencyHz : frequencies) {

        // Test a +12 dB boost at 2 kHz.
        const float boostChangeDb =
            measureFrequencyChange(frequencyHz, 2000.0f, 12.0f);

        EXPECT_LT(std::abs(boostChangeDb), maximumChangeDb)
            << "Frequency: " << frequencyHz << " Hz";

        // Test a -12 dB cut at 2 kHz.
        const float cutChangeDb =
            measureFrequencyChange(frequencyHz, 2000.0f, -12.0f);

        EXPECT_LT(std::abs(cutChangeDb), maximumChangeDb)
            << "Frequency: " << frequencyHz << " Hz";
    }
}