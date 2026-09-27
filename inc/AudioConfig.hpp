/**
 * @file
 * @brief Application audio configuration shared with regression tests.
 */
#pragma once
#include <cstddef>
#include <limits>

namespace equalizer_app::config
{
/// Application PCM sample rate, shared with test signal generation.
inline constexpr float kSampleRateHz = 48000.0f;
/// Application block size: 5.33 ms of audio at 48 kHz.
inline constexpr std::size_t kProcessingBlockSize = 256;
/// Selected band quality factor; checked by isolation and response tests.
inline constexpr float kSelectedQ = 8.0f;
static_assert(kSampleRateHz > 0.0f &&
                  kSampleRateHz <= std::numeric_limits<float>::max(),
              "Sample rate must be positive and finite.");
static_assert(kProcessingBlockSize > 0, "Processing blocks must be nonempty.");
static_assert(kSelectedQ > 0.0f &&
                  kSelectedQ <= std::numeric_limits<float>::max(),
              "Selected Q must be positive.");
} // namespace equalizer_app::config
