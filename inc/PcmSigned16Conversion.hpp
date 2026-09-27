/**
 * @file
 * @brief Conversion between signed 16-bit PCM and float
 * samples.
 */

#pragma once

#include <cstdint>

/// Stateless sample conversion for audio adapters,
/// independent of CLI and I/O. Operates on numeric samples; the file
/// adapter handles byte order.
namespace PcmSigned16Conversion
{
/**
 * @brief Normalize a signed 16-bit PCM sample.
 * @param sample PCM sample to convert.
 * @return Sample divided by 32768, in [-1, 1).
 */
float toFloat(std::int16_t sample) noexcept;

/**
 * @brief Clip and convert a float to signed 16-bit PCM.
 * @param sample Audio sample; must not be NaN.
 * @return Value in [-32768, 32767], truncated toward zero
 * after scaling. Inputs outside [-1, 1] are clipped.
 */
std::int16_t fromFloat(float sample) noexcept;
} // namespace PcmSigned16Conversion
