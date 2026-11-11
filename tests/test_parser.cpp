#include <gtest/gtest.h>

#include "parser.h"

// UT-01: plain and prefixed hexadecimal text is converted.
TEST(ParseHexValue, ParsesPlainAndPrefixedValues)
{
    uint32_t value = 0;

    EXPECT_EQ(STATUS_SUCCESS, parseHexValue("1A", 0xFF, &value));
    EXPECT_EQ(0x1Au, value);

    EXPECT_EQ(STATUS_SUCCESS, parseHexValue("0x100", 0x7FF, &value));
    EXPECT_EQ(0x100u, value);
}

// UT-02: the maximum value is accepted and one above it is rejected.
TEST(ParseHexValue, AcceptsMaximumAndRejectsAboveIt)
{
    uint32_t value = 0;

    EXPECT_EQ(STATUS_SUCCESS, parseHexValue("FF", 0xFF, &value));
    EXPECT_EQ(255u, value);

    value = 99;
    EXPECT_EQ(STATUS_ERR_OUT_OF_RANGE, parseHexValue("100", 0xFF, &value));
    EXPECT_EQ(99u, value);
}

// UT-03: text that is not hexadecimal is rejected and nothing is written.
TEST(ParseHexValue, RejectsInvalidText)
{
    uint32_t value = 99;

    EXPECT_EQ(STATUS_ERR_INVALID_HEX, parseHexValue("ZZ", 0xFF, &value));
    EXPECT_EQ(99u, value);

    EXPECT_EQ(STATUS_ERR_INVALID_HEX, parseHexValue("", 0xFF, &value));
    EXPECT_EQ(99u, value);
}