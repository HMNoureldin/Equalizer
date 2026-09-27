/**
 * @file
 * @brief Frequency-response and cross-band isolation tests.
 */

#include "AudioConfig.hpp"
#include "TestSignalUtils.hpp"
#include <audioeq/Equalizer.hpp>

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
 * @param toneHz Center frequency that should remain unchanged (either
 * configured band center).
 * @param otherBandGainDb Gain applied to the other band in decibels.
 *
 * @return Measured change of the unchanged tone in decibels.
 *
 * @note The first 0.1 seconds are ignored to allow the filters to settle.
 */
float measureBandLeakage(float q, float toneHz, float otherBandGainDb)
{
    constexpr std::size_t sampleCount =
        static_cast<std::size_t>(equalizer_app::config::kSampleRateHz);
    constexpr std::size_t blockSize =
        equalizer_app::config::kProcessingBlockSize;
    constexpr std::size_t settlingSamples = sampleCount / 10;

    // Generate the tone that should remain unchanged.
    const auto input = testutils::generateSine(toneHz, 0.1f, sampleCount);

    // Keep the original input and process a copy.
    auto output = input;

    // Start with both bands at 0 dB.
    float firstBandGainDb = 0.0f;
    float secondBandGainDb = 0.0f;

    // Apply gain only to the other band.
    if (toneHz == equalizer_app::config::kFirstBandFrequencyHz) {
        secondBandGainDb = otherBandGainDb;
    } else {
        firstBandGainDb = otherBandGainDb;
    }

    audioeq::Equalizer equalizer(
        equalizer_app::config::kSampleRateHz,
        {equalizer_app::config::kFirstBandFrequencyHz, q, firstBandGainDb},
        {equalizer_app::config::kSecondBandFrequencyHz, q, secondBandGainDb});

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
 * @param centerHz EQ band to change (either configured band center).
 * @param gainDb Gain applied to the selected band.
 *
 * @return Measured change of the test tone in decibels.
 *
 * @note The first 0.1 seconds are ignored to allow the filters to settle.
 */
float measureFrequencyChange(float toneHz, float centerHz, float gainDb)
{
    constexpr std::size_t sampleCount =
        static_cast<std::size_t>(equalizer_app::config::kSampleRateHz);
    constexpr std::size_t blockSize =
        equalizer_app::config::kProcessingBlockSize;
    constexpr std::size_t settlingSamples = sampleCount / 10;

    // Generate the frequency that we want to measure.
    const auto input = testutils::generateSine(toneHz, 0.1f, sampleCount);

    // Keep the original input and process a copy.
    auto output = input;

    // Start with both bands at 0 dB.
    float firstBandGainDb = 0.0f;
    float secondBandGainDb = 0.0f;

    // Apply gain only to the selected band.
    if (centerHz == equalizer_app::config::kFirstBandFrequencyHz) {
        firstBandGainDb = gainDb;
    } else {
        secondBandGainDb = gainDb;
    }

    audioeq::Equalizer equalizer(
        equalizer_app::config::kSampleRateHz,
        {equalizer_app::config::kFirstBandFrequencyHz,
         equalizer_app::config::kSelectedQ, firstBandGainDb},
        {equalizer_app::config::kSecondBandFrequencyHz,
         equalizer_app::config::kSelectedQ, secondBandGainDb});

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

/// Boosting the second band must change the first band by less than 0.5 dB.
TEST(EqualizerIsolation, BoostSecondBandDoesNotAffectFirstBand)
{
    constexpr float isolationLimitDb = 0.5f;

    const float leakageDb =
        measureBandLeakage(equalizer_app::config::kSelectedQ,
                           equalizer_app::config::kFirstBandFrequencyHz,
                           audioeq::Equalizer::kMaxGainDb);

    EXPECT_LT(std::abs(leakageDb), isolationLimitDb);
}

/// Cutting the second band must change the first band by less than 0.5 dB.
TEST(EqualizerIsolation, CutSecondBandDoesNotAffectFirstBand)
{
    constexpr float isolationLimitDb = 0.5f;

    const float leakageDb =
        measureBandLeakage(equalizer_app::config::kSelectedQ,
                           equalizer_app::config::kFirstBandFrequencyHz,
                           audioeq::Equalizer::kMinGainDb);

    EXPECT_LT(std::abs(leakageDb), isolationLimitDb);
}

/// Boosting the first band must change the second band by less than 0.5 dB.
TEST(EqualizerIsolation, BoostFirstBandDoesNotAffectSecondBand)
{
    constexpr float isolationLimitDb = 0.5f;

    const float leakageDb =
        measureBandLeakage(equalizer_app::config::kSelectedQ,
                           equalizer_app::config::kSecondBandFrequencyHz,
                           audioeq::Equalizer::kMaxGainDb);

    EXPECT_LT(std::abs(leakageDb), isolationLimitDb);
}

/// Cutting the first band must change the second band by less than 0.5 dB.
TEST(EqualizerIsolation, CutFirstBandDoesNotAffectSecondBand)
{
    constexpr float isolationLimitDb = 0.5f;

    const float leakageDb =
        measureBandLeakage(equalizer_app::config::kSelectedQ,
                           equalizer_app::config::kSecondBandFrequencyHz,
                           audioeq::Equalizer::kMinGainDb);

    EXPECT_LT(std::abs(leakageDb), isolationLimitDb);
}

// -----------------------------------------------------------------------------
// Neighbor-frequency tests
// -----------------------------------------------------------------------------

/**
 * @brief Verify that changing the first band has little effect on
 *        frequencies far away from it.
 *
 * Each frequency is tested with the maximum boost and maximum cut.
 */
TEST(EqualizerNeighborResponse, FirstBandDoesNotAffectDistantFrequencies)
{
    constexpr float maximumChangeDb = 0.5f;

    // Frequencies away from the first band.
    const float frequencies[] = {250.0f, 500.0f, 3000.0f, 4000.0f, 6000.0f};

    for (const float frequencyHz : frequencies) {

        // Test a +12 dB boost at the first band.
        const float boostChangeDb = measureFrequencyChange(
            frequencyHz, equalizer_app::config::kFirstBandFrequencyHz,
            audioeq::Equalizer::kMaxGainDb);

        EXPECT_LT(std::abs(boostChangeDb), maximumChangeDb)
            << "Frequency: " << frequencyHz << " Hz";

        // Test a -12 dB cut at the first band.
        const float cutChangeDb = measureFrequencyChange(
            frequencyHz, equalizer_app::config::kFirstBandFrequencyHz,
            audioeq::Equalizer::kMinGainDb);

        EXPECT_LT(std::abs(cutChangeDb), maximumChangeDb)
            << "Frequency: " << frequencyHz << " Hz";
    }
}

/**
 * @brief Verify that changing the second band has little effect on
 *        frequencies far away from it.
 *
 * Each frequency is tested with the maximum boost and maximum cut.
 */
TEST(EqualizerNeighborResponse, SecondBandDoesNotAffectDistantFrequencies)
{
    constexpr float maximumChangeDb = 0.5f;

    // Frequencies away from the second band.
    const float frequencies[] = {250.0f, 500.0f, 4000.0f, 6000.0f};

    for (const float frequencyHz : frequencies) {

        // Test a +12 dB boost at the second band.
        const float boostChangeDb = measureFrequencyChange(
            frequencyHz, equalizer_app::config::kSecondBandFrequencyHz,
            audioeq::Equalizer::kMaxGainDb);

        EXPECT_LT(std::abs(boostChangeDb), maximumChangeDb)
            << "Frequency: " << frequencyHz << " Hz";

        // Test a -12 dB cut at the second band.
        const float cutChangeDb = measureFrequencyChange(
            frequencyHz, equalizer_app::config::kSecondBandFrequencyHz,
            audioeq::Equalizer::kMinGainDb);

        EXPECT_LT(std::abs(cutChangeDb), maximumChangeDb)
            << "Frequency: " << frequencyHz << " Hz";
    }
}