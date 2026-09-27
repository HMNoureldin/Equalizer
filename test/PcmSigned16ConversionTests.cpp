/**
 * @file
 * @brief Tests for PCM sample conversion and clipping.
 */

#include "PcmSigned16Conversion.hpp"

#include <cstdint>

#include <gtest/gtest.h>

/**
 * @brief Verify that values above positive full scale are clipped.
 */
TEST(PcmSigned16ConversionTests, ClipsPositiveValuesAboveFullScale)
{
    const std::int16_t output = PcmSigned16Conversion::fromFloat(2.0f);

    EXPECT_EQ(output, 32767);
}

/**
 * @brief Verify that values below negative full scale are clipped.
 */
TEST(PcmSigned16ConversionTests, ClipsNegativeValuesBelowFullScale)
{
    const std::int16_t output = PcmSigned16Conversion::fromFloat(-2.0f);

    EXPECT_EQ(output, -32768);
}

/**
 * @brief Verify conversion at the exact full-scale boundaries.
 */
TEST(PcmSigned16ConversionTests, ConvertsFullScaleBoundaries)
{
    EXPECT_EQ(PcmSigned16Conversion::fromFloat(1.0f), 32767);

    EXPECT_EQ(PcmSigned16Conversion::fromFloat(-1.0f), -32768);
}