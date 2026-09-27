/**
 * @file
 * @brief Compile-time gain limits for the standalone DSP library.
 * @note Rebuild the library and all consumers after changing these values.
 */
#pragma once

#include <limits>

namespace audioeq::config
{
/// Minimum supported band gain in decibels.
inline constexpr float kMinGainDb = -12.0f;
/// Maximum supported band gain in decibels.
inline constexpr float kMaxGainDb = 12.0f;

static_assert(kMinGainDb >= -std::numeric_limits<float>::max() &&
                  kMaxGainDb <= std::numeric_limits<float>::max() &&
                  kMinGainDb <= 0.0f && kMaxGainDb >= 0.0f &&
                  kMinGainDb < kMaxGainDb,
              "Gain limits must be finite, ordered, and include zero.");
} // namespace audioeq::config
