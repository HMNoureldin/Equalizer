/**
 * @file
 * @brief Tests for PCM sample conversion and clipping.
 */

#include "PcmConversion.hpp"

#include <cstdint>

#include <gtest/gtest.h>

/**
 * @brief Verify that values above positive full scale are clipped.
 */
TEST(PcmConversionTests, ClipsPositiveValuesAboveFullScale)
{
    const std::int16_t output = PcmConversion::floatToPcm16(2.0f);

    EXPECT_EQ(output, 32767);
}

/**
 * @brief Verify that values below negative full scale are clipped.
 */
TEST(PcmConversionTests, ClipsNegativeValuesBelowFullScale)
{
    const std::int16_t output = PcmConversion::floatToPcm16(-2.0f);

    EXPECT_EQ(output, -32768);
}

/**
 * @brief Verify conversion at the exact full-scale boundaries.
 */
TEST(PcmConversionTests, ConvertsFullScaleBoundaries)
{
    EXPECT_EQ(PcmConversion::floatToPcm16(1.0f), 32767);

    EXPECT_EQ(PcmConversion::floatToPcm16(-1.0f), -32768);
}