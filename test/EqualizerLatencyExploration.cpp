/**
 * @file
 * @brief Exploratory measurements for Equalizer latency at different
 *        processing block sizes.
 *
 * This file is used to compare candidate block sizes and help justify
 * the application's selected processing block size. It is not a
 * correctness test for a particular block size.
 */

#include "AudioConfig.hpp"
#include "TestSignalUtils.hpp"
#include <audioeq/Equalizer.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iostream>

#include <gtest/gtest.h>

namespace
{

/// Reference target; the printed estimate excludes other latency contributions.
constexpr double kMaximumLatencyMs = 100.0;
/// Number of observed processing calls for each candidate buffer size.
constexpr std::size_t kMeasurementCount = 100;

/**
 * @brief Measure the latency budget for a candidate block size.
 *
 * The reported budget consists of:
 *   - the duration of one audio block;
 *   - the worst observed Equalizer processing time.
 *
 * @param blockSize Number of samples in one processing block.
 */
void reportLatency(std::size_t blockSize)
{
#ifndef NDEBUG
    std::cout << "Note: Debug build; use BUILD_TYPE=Release for timing.\n";
#endif
    std::cout << "Excludes output buffering, conversion, filter delay, and "
                 "device overhead.\n";
    // Calculate how much audio one block represents.
    const double bufferLatencyMs =
        static_cast<double>(blockSize) /
        static_cast<double>(equalizer_app::config::kSampleRateHz) * 1000.0;

    // Create a representative signal containing both EQ frequencies.
    auto buffer = testutils::generateSine(1000.0f, 0.1f, blockSize);

    const auto secondTone = testutils::generateSine(2000.0f, 0.1f, blockSize);

    for (std::size_t i = 0; i < blockSize; ++i) {
        buffer[i] += secondTone[i];
    }

    // Use non-zero gains so both EQ bands are active.
    audioeq::Equalizer equalizer(
        equalizer_app::config::kSampleRateHz,
        {1000.0f, equalizer_app::config::kSelectedQ, 6.0f},
        {2000.0f, equalizer_app::config::kSelectedQ, -3.0f});

    const auto input = buffer;
    double worstProcessingTimeMs = 0.0;

    // Measure several calls to reduce dependence on one timing sample.
    for (std::size_t i = 0; i < kMeasurementCount; ++i) {
        // Refresh input outside timing; do not repeatedly boost old output.
        std::copy(input.begin(), input.end(), buffer.begin());
        const auto start = std::chrono::steady_clock::now();

        equalizer.process(buffer.data(), buffer.size());

        const auto end = std::chrono::steady_clock::now();

        const double processingTimeMs =
            std::chrono::duration<double, std::milli>(end - start).count();

        worstProcessingTimeMs =
            std::max(worstProcessingTimeMs, processingTimeMs);
    }

    const double latencyBudgetMs = bufferLatencyMs + worstProcessingTimeMs;

    std::cout << "Block size: " << blockSize << " samples\n"
              << "  Buffer latency: " << bufferLatencyMs << " ms\n"
              << "  Maximum observed processing time: " << worstProcessingTimeMs
              << " ms\n"
              << "  One input block + DSP estimate: " << latencyBudgetMs
              << " ms\n"
              << "  Reference target (not fully assessed here): < "
              << kMaximumLatencyMs << " ms\n\n";
}

} // namespace

/// Print the partial latency estimate for 256-sample blocks.
TEST(EqualizerLatencyExploration, Block256)
{
    reportLatency(256);
}

/// Print the partial latency estimate for 512-sample blocks.
TEST(EqualizerLatencyExploration, Block512)
{
    reportLatency(512);
}

/// Print the partial latency estimate for 1024-sample blocks.
TEST(EqualizerLatencyExploration, Block1024)
{
    reportLatency(1024);
}