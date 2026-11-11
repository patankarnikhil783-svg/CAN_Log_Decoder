// Unit tests for the parser module (UT-01 to UT-07).

#include <cstring>

#include <gtest/gtest.h>

#include "parser.h"

static const char *VALID_LINE = "1000,0x100,8,1A,2C,78,80,00,00,00,CE";

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

// UT-04: a valid line, with and without a line ending, gives the right frame. 
TEST(ParseFrameLine, ParsesValidLineWithAndWithoutLineEnding)
{
    CanFrame_t frame;
    const uint8_t expected[DATA_BYTES] = {0x1A, 0x2C, 0x78, 0x80, 0x00, 0x00, 0x00, 0xCE};

    EXPECT_EQ(STATUS_SUCCESS, parseFrameLine(VALID_LINE, &frame));
    EXPECT_EQ(1000u, frame.timeMs);
    EXPECT_EQ(0x100, frame.id);
    EXPECT_EQ(8, frame.dlc);
    EXPECT_EQ(0, memcmp(frame.data, expected, DATA_BYTES));

    EXPECT_EQ(STATUS_SUCCESS,
              parseFrameLine("1000,0x100,8,1A,2C,78,80,00,00,00,CE\r\n", &frame));
    EXPECT_EQ(0xCE, frame.data[CHECKSUM_INDEX]);
}

// UT-05: a line with too few or too many fields is rejected.
TEST(ParseFrameLine, RejectsWrongFieldCount)
{
    CanFrame_t frame;

    EXPECT_EQ(STATUS_ERR_FIELD_COUNT, parseFrameLine("1000,0x100,8,1A,2C", &frame));
    EXPECT_EQ(STATUS_ERR_FIELD_COUNT,
              parseFrameLine("1000,0x100,8,1A,2C,78,80,00,00,00,CE,FF", &frame));
}

// UT-06: a data byte that is not hexadecimal is rejected, frame unchanged. 
TEST(ParseFrameLine, RejectsInvalidDataByte)
{
    CanFrame_t frame;

    frame.timeMs = 77;
    EXPECT_EQ(STATUS_ERR_INVALID_HEX,
              parseFrameLine("1000,0x100,8,ZZ,2C,78,80,00,00,00,CE", &frame));
    EXPECT_EQ(77u, frame.timeMs);
}

// UT-07: the limits of the identifier and the data length.
TEST(ParseFrameLine, EnforcesIdentifierAndLengthLimits)
{
    CanFrame_t frame;

    EXPECT_EQ(STATUS_SUCCESS,
              parseFrameLine("1000,0x7FF,8,1A,2C,78,80,00,00,00,CE", &frame));
    EXPECT_EQ(STATUS_ERR_OUT_OF_RANGE,
              parseFrameLine("1000,0x800,8,1A,2C,78,80,00,00,00,CE", &frame));
    EXPECT_EQ(STATUS_ERR_OUT_OF_RANGE,
              parseFrameLine("1000,0x100,9,1A,2C,78,80,00,00,00,CE", &frame));
}