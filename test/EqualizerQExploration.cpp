/**
 * @file
 * @brief Exploratory measurements for selecting the Equalizer Q value.
 *
 * This file is used to compare candidate Q values before selecting the
 * application Q. It prints band-isolation measurements but does not enforce
 * pass/fail isolation requirements.
 */

#include <audioeq/Equalizer.hpp>
#include "TestSignalUtils.hpp"

#include <algorithm>
#include <cstddef>
#include <gtest/gtest.h>
#include <iostream>

namespace
{

/**
 * @brief Measure how much changing one EQ band affects the other band's
 *        center frequency.
 *
 * @param q Quality factor used by both EQ bands.
 * @param toneHz Tone that should remain unchanged (1000 or 2000 Hz).
 * @param otherBandGainDb Gain applied to the other EQ band.
 *
 * @return Measured change of the test tone in dB.
 */
float measureBandLeakage(float q, float toneHz, float otherBandGainDb)
{
    constexpr std::size_t sampleCount = 48000;
    constexpr std::size_t blockSize = 1024;
    constexpr std::size_t settlingSamples = 4800;

    // Generate the tone that should remain unchanged.
    const auto input = testutils::generateSine(toneHz, 0.1f, sampleCount);

    // Keep the original input and process a copy.
    auto output = input;

    // Initially both bands have zero gain.
    float gain1000Hz = 0.0f;
    float gain2000Hz = 0.0f;

    // Apply the requested gain only to the OTHER band.
    if (toneHz == 1000.0f) {
        gain2000Hz = otherBandGainDb;
    } else {
        gain1000Hz = otherBandGainDb;
    }

    // Create the equalizer using the Q value being investigated.
    audioeq::Equalizer equalizer(testutils::kSampleRateHz,
                                 {1000.0f, q, gain1000Hz},
                                 {2000.0f, q, gain2000Hz});

    // Process the signal in blocks.
    for (std::size_t offset = 0; offset < sampleCount; offset += blockSize) {

        equalizer.process(output.data() + offset,
                          std::min(blockSize, sampleCount - offset));
    }

    // Measure the input and output after the filter has settled.
    const float inputRms = testutils::calculateRms(input, settlingSamples);

    const float outputRms = testutils::calculateRms(output, settlingSamples);

    // Convert the change in RMS level to decibels.
    return testutils::amplitudeRatioToDb(outputRms / inputRms);
}

} // namespace

/**
 * @brief Print isolation measurements for candidate Q values.
 *
 * No isolation requirement is asserted here. The purpose of this test is
 * exploratory: run the candidates, inspect their measured leakage, and use
 * those measurements to justify the Q selected by the application.
 */
TEST(EqualizerQExploration, PrintIsolationResults)
{
    const float candidateQValues[] = {2.0f, 4.0f, 8.0f};

    for (const float q : candidateQValues) {

        std::cout << "\n===== Q = " << q << " =====\n";

        std::cout << "1000 Hz tone, 2000 Hz band +12 dB: "
                  << measureBandLeakage(q, 1000.0f, 12.0f) << " dB\n";

        std::cout << "1000 Hz tone, 2000 Hz band -12 dB: "
                  << measureBandLeakage(q, 1000.0f, -12.0f) << " dB\n";

        std::cout << "2000 Hz tone, 1000 Hz band +12 dB: "
                  << measureBandLeakage(q, 2000.0f, 12.0f) << " dB\n";

        std::cout << "2000 Hz tone, 1000 Hz band -12 dB: "
                  << measureBandLeakage(q, 2000.0f, -12.0f) << " dB\n";
    }
}