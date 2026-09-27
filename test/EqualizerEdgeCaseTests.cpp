/**
 * @file
 * @brief Edge-case tests for the Equalizer.
 */

#include "AudioConfig.hpp"
#include "TestSignalUtils.hpp"
#include <audioeq/Equalizer.hpp>

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

/**
 * @brief Verify that maximum EQ boost does not create sound from silence.
 */
TEST(EqualizerEdgeCases, SilenceRemainsSilent)
{
    // Create one processing block containing only silence.
    std::vector<float> samples(equalizer_app::config::kProcessingBlockSize,
                               0.0f);

    // Use maximum boost on both EQ bands.
    audioeq::Equalizer equalizer(
        equalizer_app::config::kSampleRateHz,
        {equalizer_app::config::kFirstBandFrequencyHz,
         equalizer_app::config::kSelectedQ, audioeq::Equalizer::kMaxGainDb},
        {equalizer_app::config::kSecondBandFrequencyHz,
         equalizer_app::config::kSelectedQ, audioeq::Equalizer::kMaxGainDb});

    // Process the silent block.
    equalizer.process(samples.data(), samples.size());

    // Silence should remain silence.
    for (const float sample : samples) {
        EXPECT_FLOAT_EQ(sample, 0.0f);
    }
}

/**
 * @brief Verify that both bands accept the inclusive gain boundaries.
 * @note Checks all four pairings of the minimum and maximum allowed gains.
 * This verifies constructor acceptance, not measured audio gain.
 */
TEST(EqualizerEdgeCases, AcceptsMaximumAllowedGains)
{
    constexpr float gainLimits[] = {audioeq::Equalizer::kMinGainDb,
                                    audioeq::Equalizer::kMaxGainDb};

    for (const float firstGainDb : gainLimits) {
        for (const float secondGainDb : gainLimits) {
            EXPECT_NO_THROW(audioeq::Equalizer(
                equalizer_app::config::kSampleRateHz,
                {equalizer_app::config::kFirstBandFrequencyHz,
                 equalizer_app::config::kSelectedQ, firstGainDb},
                {equalizer_app::config::kSecondBandFrequencyHz,
                 equalizer_app::config::kSelectedQ, secondGainDb}))
                << "First band gain: " << firstGainDb
                << " dB; second band gain: " << secondGainDb << " dB";
        }
    }
}

/**
 * @brief Verify that either band rejects gains outside the allowed range.
 * @note Tests one dB below the minimum and above the maximum, with the
 * other band at a valid zero gain. Construction must throw invalid_argument.
 */
TEST(EqualizerEdgeCases, RejectsGainOutsideAllowedRange)
{
    constexpr float invalidGains[] = {audioeq::Equalizer::kMinGainDb - 1.0f,
                                      audioeq::Equalizer::kMaxGainDb + 1.0f};

    for (const float gainDb : invalidGains) {
        EXPECT_THROW(
            audioeq::Equalizer(equalizer_app::config::kSampleRateHz,
                               {equalizer_app::config::kFirstBandFrequencyHz,
                                equalizer_app::config::kSelectedQ, gainDb},
                               {equalizer_app::config::kSecondBandFrequencyHz,
                                equalizer_app::config::kSelectedQ, 0.0f}),
            std::invalid_argument)
            << "First band gain: " << gainDb << " dB";

        EXPECT_THROW(
            audioeq::Equalizer(equalizer_app::config::kSampleRateHz,
                               {equalizer_app::config::kFirstBandFrequencyHz,
                                equalizer_app::config::kSelectedQ, 0.0f},
                               {equalizer_app::config::kSecondBandFrequencyHz,
                                equalizer_app::config::kSelectedQ, gainDb}),
            std::invalid_argument)
            << "Second band gain: " << gainDb << " dB";
    }
}

/**
 * @brief Verify that a zero sample count leaves the supplied data unchanged.
 * @note A valid pointer is supplied to isolate the zero-length behavior.
 */
TEST(EqualizerEdgeCases, EmptyBufferDoesNotChangeData)
{
    audioeq::Equalizer equalizer(equalizer_app::config::kSampleRateHz,
                                 {equalizer_app::config::kFirstBandFrequencyHz,
                                  equalizer_app::config::kSelectedQ, 6.0f},
                                 {equalizer_app::config::kSecondBandFrequencyHz,
                                  equalizer_app::config::kSelectedQ, -6.0f});

    float sample = 0.25f;
    equalizer.process(&sample, 0);

    EXPECT_FLOAT_EQ(sample, 0.25f);
}

/**
 * @brief Verify that both bands reject NaN and infinite gains.
 * @note Tests NaN and both signs of infinity independently on each band.
 * Construction must throw std::invalid_argument.
 */
TEST(EqualizerEdgeCases, RejectsInvalidGainValues)
{
    const float invalidGains[] = {std::numeric_limits<float>::quiet_NaN(),
                                  std::numeric_limits<float>::infinity(),
                                  -std::numeric_limits<float>::infinity()};

    for (const float gainDb : invalidGains) {
        EXPECT_THROW(
            audioeq::Equalizer(equalizer_app::config::kSampleRateHz,
                               {equalizer_app::config::kFirstBandFrequencyHz,
                                equalizer_app::config::kSelectedQ, gainDb},
                               {equalizer_app::config::kSecondBandFrequencyHz,
                                equalizer_app::config::kSelectedQ, 0.0f}),
            std::invalid_argument)
            << "First band gain: " << gainDb;

        EXPECT_THROW(
            audioeq::Equalizer(equalizer_app::config::kSampleRateHz,
                               {equalizer_app::config::kFirstBandFrequencyHz,
                                equalizer_app::config::kSelectedQ, 0.0f},
                               {equalizer_app::config::kSecondBandFrequencyHz,
                                equalizer_app::config::kSelectedQ, gainDb}),
            std::invalid_argument)
            << "Second band gain: " << gainDb;
    }
}

/**
 * @brief Verify that reset restores fresh filter history without changing
 * the band configuration.
 * @note Identical input and configuration must produce exactly equal output
 * from a reset instance and a newly constructed instance.
 */
TEST(EqualizerEdgeCases, ResetMatchesFreshEqualizer)
{
    constexpr std::size_t sampleCount = 1000;
    constexpr float toneAmplitude = 0.1f;
    constexpr audioeq::Equalizer::BandConfig firstBandConfig{
        equalizer_app::config::kFirstBandFrequencyHz,
        equalizer_app::config::kSelectedQ, 6.0f};
    constexpr audioeq::Equalizer::BandConfig secondBandConfig{
        equalizer_app::config::kSecondBandFrequencyHz,
        equalizer_app::config::kSelectedQ, -6.0f};

    // Accumulate history, then reset the equalizer under test.
    audioeq::Equalizer resetEqualizer(equalizer_app::config::kSampleRateHz,
                                      firstBandConfig, secondBandConfig);
    auto historySamples = testutils::generateSine(firstBandConfig.frequencyHz,
                                                  toneAmplitude, sampleCount);
    resetEqualizer.process(historySamples.data(), historySamples.size());
    resetEqualizer.reset();

    // Create a fresh reference with the same band settings.
    audioeq::Equalizer freshEqualizer(equalizer_app::config::kSampleRateHz,
                                      firstBandConfig, secondBandConfig);
    const auto inputSamples = testutils::generateSine(
        secondBandConfig.frequencyHz, toneAmplitude, sampleCount);
    auto resetOutput = inputSamples;
    auto freshOutput = inputSamples;

    // Process identical samples through both instances.
    resetEqualizer.process(resetOutput.data(), resetOutput.size());
    freshEqualizer.process(freshOutput.data(), freshOutput.size());

    EXPECT_EQ(resetOutput, freshOutput);
}
