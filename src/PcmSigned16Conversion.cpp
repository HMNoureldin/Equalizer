/**
 * @file
 * @brief PCM normalization and clipping for audio adapters.
 */

#include "PcmSigned16Conversion.hpp"

#include <algorithm>

namespace PcmSigned16Conversion
{
float toFloat(std::int16_t sample) noexcept
{
    return static_cast<float>(sample) / 32768.0f;
}

std::int16_t fromFloat(float sample) noexcept
{
    const float clamped = std::clamp(sample, -1.0f, 1.0f);
    if (clamped >= 1.0f) {
        return 32767;
    }
    return static_cast<std::int16_t>(clamped * 32768.0f);
}
} // namespace PcmSigned16Conversion
