/**
 * @file
 * @brief Application audio configuration shared with regression tests.
 */
#pragma once
#include <cstddef>

namespace audioeq
{
/// Application block size: 5.33 ms of audio at 48 kHz.
inline constexpr std::size_t kProcessingBlockSize = 256;
/// Selected band quality factor; checked by isolation and response tests.
inline constexpr float kSelectedQ = 8.0f;
} // namespace audioeq
