/**
 * @file
 * @brief Tests for the Equalizer latency budget.
 */

#include "AudioConfig.hpp"
#include "Equalizer.hpp"
#include "TestSignalUtils.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>

#include <gtest/gtest.h>

namespace
{

/// Budget for this partial buffering-plus-DSP check.
constexpr double kMaximumLatencyMs = 100.0;
/// Number of timed calls.
constexpr std::size_t kMeasurementCount = 100;

/**
 * @brief Calculate the duration of one audio block in milliseconds.
 * @param blockSize Number of samples per block.
 * @return Block duration in milliseconds.
 */
double calculateBufferLatencyMs(std::size_t blockSize)
{
    return static_cast<double>(blockSize) /
           static_cast<double>(testutils::kSampleRateHz) * 1000.0;
}

} // namespace

/**
 * @brief Verify that buffering plus measured Equalizer processing time
 *        stays below the 100 ms latency budget.
 *
 * The selected application block size determines the buffering contribution.
 * The Equalizer is then executed repeatedly and the worst observed processing
 * time is used as an observed processing-time contribution.
 * @note Excludes output buffering, conversion, filter group delay, and device
 * overhead. Passing does not establish total end-to-end latency or worst-case
 * time.
 */
TEST(EqualizerLatency, SelectedBlockSizeStaysBelow100Milliseconds)
{
    // Amount of audio collected before processing one block.
    const double bufferLatencyMs =
        calculateBufferLatencyMs(audioeq::kProcessingBlockSize);

    // Create one representative block containing both EQ frequencies.
    auto buffer =
        testutils::generateSine(1000.0f, 0.1f, audioeq::kProcessingBlockSize);

    const auto secondTone =
        testutils::generateSine(2000.0f, 0.1f, audioeq::kProcessingBlockSize);

    for (std::size_t i = 0; i < buffer.size(); ++i) {
        buffer[i] += secondTone[i];
    }

    // Use representative non-zero settings so both bands do work.
    audioeq::Equalizer equalizer(testutils::kSampleRateHz,
                                 {1000.0f, audioeq::kSelectedQ, 6.0f},
                                 {2000.0f, audioeq::kSelectedQ, -3.0f});

    const auto input = buffer;
    double worstProcessingTimeMs = 0.0;

    // Measure several blocks because a single timing measurement
    // can be affected by normal system timing variation.
    for (std::size_t i = 0; i < kMeasurementCount; ++i) {
        // Refresh outside timing rather than repeatedly boosting old output.
        std::copy(input.begin(), input.end(), buffer.begin());
        const auto start = std::chrono::steady_clock::now();

        equalizer.process(buffer.data(), buffer.size());

        const auto end = std::chrono::steady_clock::now();

        const double processingTimeMs =
            std::chrono::duration<double, std::milli>(end - start).count();

        worstProcessingTimeMs =
            std::max(worstProcessingTimeMs, processingTimeMs);
    }

    // Partial latency estimate (not a full end-to-end budget):
    // buffering time + worst measured DSP execution time.
    const double latencyBudgetMs = bufferLatencyMs + worstProcessingTimeMs;

    EXPECT_LT(latencyBudgetMs, kMaximumLatencyMs);
}