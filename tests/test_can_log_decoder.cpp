// Unit tests for the parser module (UT-01 to UT-07).
#include <gtest/gtest.h>

#include <cstring>
#include <set>
#include <string>

#include "types.h"
#include "menu.h"
#include "file_handler.h"
#include "parser.h"
#include "validation.h"
#include "decoder.h"
#include "frame_store.h"
#include "report.h"

//test_parser
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

//test_validation
static CanFrame_t validEngineFrame()
{
    CanFrame_t frame = {};
    frame.timeMs = 1000U;
    frame.id = ID_ENGINE_DATA;
    frame.dlc = DATA_BYTES;
    const uint8_t bytes[DATA_BYTES] = {0x1A, 0x2C, 0x78, 0x80, 0, 0, 0, 0xCE};
    for (uint8_t i = 0U; i < DATA_BYTES; i++) frame.data[i] = bytes[i];
    return frame;
}

// UT-08: XOR checksum is calculated from the first seven bytes.
TEST(CalculateChecksum, CalculatesSevenByteXor)
{
    const uint8_t data[DATA_BYTES] = {0x1A, 0x2C, 0x78, 0x80, 0, 0, 0, 0xFF};
    uint8_t checksum = 0U;
    EXPECT_EQ(STATUS_SUCCESS, calculateChecksum(data, &checksum));
    EXPECT_EQ(0xCEU, checksum);
    const uint8_t zeros[DATA_BYTES] = {};
    EXPECT_EQ(STATUS_SUCCESS, calculateChecksum(zeros, &checksum));
    EXPECT_EQ(0U, checksum);
}

// UT-09: a known eight-byte frame with the expected checksum passes.
TEST(ValidateFrame, AcceptsValidEngineFrame)
{
    CanFrame_t frame = validEngineFrame();
    EXPECT_EQ(STATUS_SUCCESS, validateFrame(&frame));
}

// UT-10: an identifier absent from the signal table is rejected.
TEST(ValidateFrame, RejectsUnknownIdentifier)
{
    CanFrame_t frame = validEngineFrame();
    frame.id = 0x7FFU;
    EXPECT_EQ(STATUS_ERR_UNKNOWN_ID, validateFrame(&frame));
}

// UT-11: the data length must be exactly eight.
TEST(ValidateFrame, RejectsWrongDataLength)
{
    CanFrame_t frame = validEngineFrame();
    frame.dlc = 7U;
    EXPECT_EQ(STATUS_ERR_BAD_LENGTH, validateFrame(&frame));
}

// UT-12: a checksum mismatch is detected. */
TEST(ValidateFrame, RejectsBadChecksum)
{
    CanFrame_t frame = validEngineFrame();
    frame.data[7] = 0U;
    EXPECT_EQ(STATUS_ERR_BAD_CHECKSUM, validateFrame(&frame));
}

// UT-13: identifier validation takes priority over checksum validation. */
TEST(ValidateFrame, ChecksUnknownIdentifierBeforeChecksum)
{
    CanFrame_t frame = validEngineFrame();
    frame.id = 0x7FFU;
    frame.data[7] = 0U;
    EXPECT_EQ(STATUS_ERR_UNKNOWN_ID, validateFrame(&frame));
}

static CanFrame_t engineFrame()
{
    CanFrame_t frame = {};
    frame.id = ID_ENGINE_DATA;
    frame.dlc = DATA_BYTES;
    const uint8_t bytes[DATA_BYTES] = {0x1A, 0x2C, 0x78, 0x80, 0, 0, 0, 0xCE};
    for (uint8_t i = 0U; i < DATA_BYTES; i++) frame.data[i] = bytes[i];
    return frame;
}

//test_decoder
// UT-14: a sixteen-bit little-endian field combines data bytes zero and one. */
TEST(ExtractBits, ReadsTwoByteSignal)
{
    CanFrame_t frame = engineFrame();
    uint32_t raw = 0U;
    EXPECT_EQ(STATUS_SUCCESS, extractBits(frame.data, 0U, 16U, &raw));
    EXPECT_EQ(0x2C1AU, raw);
}

// UT-15: bit-level brake and gear fields are extracted from byte two. */
TEST(ExtractBits, ReadsSingleAndThreeBitFields)
{
    const uint8_t data[DATA_BYTES] = {0xE8, 0x03, 0x03, 0, 0, 0, 0, 0xE8};
    uint32_t raw = 0U;
    EXPECT_EQ(STATUS_SUCCESS, extractBits(data, 16U, 1U, &raw));
    EXPECT_EQ(1U, raw);
    EXPECT_EQ(STATUS_SUCCESS, extractBits(data, 17U, 3U, &raw));
    EXPECT_EQ(1U, raw);
}

// UT-16: a field ending at bit 63 succeeds, while one extending past it fails. */
TEST(ExtractBits, EnforcesLastBitBoundary)
{
    CanFrame_t frame = engineFrame();
    uint32_t raw = 0U;
    EXPECT_EQ(STATUS_SUCCESS, extractBits(frame.data, 56U, 8U, &raw));
    EXPECT_EQ(0xCEU, raw);
    EXPECT_EQ(STATUS_ERR_INVALID_ARGUMENT, extractBits(frame.data, 56U, 9U, &raw));
}

// UT-17: zero-length fields and NULL data are rejected. */
TEST(ExtractBits, RejectsInvalidLengthAndNullPointer)
{
    CanFrame_t frame = engineFrame();
    uint32_t raw = 123U;
    EXPECT_EQ(STATUS_ERR_INVALID_ARGUMENT, extractBits(frame.data, 0U, 0U, &raw));
    EXPECT_EQ(123U, raw);
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, extractBits(nullptr, 0U, 8U, &raw));
}

// UT-18: raw values are converted using factor and offset. */
TEST(ConvertRawValue, AppliesFactorAndOffset)
{
    EXPECT_NEAR(2822.5F, convertRawValue(11290U, 0.25F, 0.0F), 0.001F);
    EXPECT_NEAR(-40.0F, convertRawValue(0U, 1.0F, -40.0F), 0.001F);
}

// UT-19: engine message yields speed, coolant temperature and throttle. */
TEST(DecodeFrame, DecodesEngineSignals)
{
    CanFrame_t frame = engineFrame();
    DecodedValues_t decoded = {};
    EXPECT_EQ(STATUS_SUCCESS, decodeFrame(&frame, &decoded));
    ASSERT_EQ(3U, decoded.count);
    EXPECT_NEAR(2822.5F, decoded.values[0], 0.001F);
    EXPECT_NEAR(80.0F, decoded.values[1], 0.001F);
    EXPECT_NEAR(51.2F, decoded.values[2], 0.001F);
}

// UT-20: vehicle and battery message signal definitions decode correctly. */
TEST(DecodeFrame, DecodesVehicleAndBatterySignals)
{
    CanFrame_t frame = {};
    DecodedValues_t decoded = {};
    frame.id = ID_VEHICLE_SPEED;
    frame.dlc = DATA_BYTES;
    const uint8_t vehicle[DATA_BYTES] = {0xE8, 0x03, 0x03, 0, 0, 0, 0, 0xE8};
    for (uint8_t i = 0U; i < DATA_BYTES; i++) frame.data[i] = vehicle[i];
    EXPECT_EQ(STATUS_SUCCESS, decodeFrame(&frame, &decoded));
    ASSERT_EQ(3U, decoded.count);
    EXPECT_NEAR(10.0F, decoded.values[0], 0.001F);
    EXPECT_NEAR(1.0F, decoded.values[1], 0.001F);
    EXPECT_NEAR(1.0F, decoded.values[2], 0.001F);
    frame.id = ID_BATTERY_STATUS;
    frame.data[0] = 0x7B;
    EXPECT_EQ(STATUS_SUCCESS, decodeFrame(&frame, &decoded));
    ASSERT_EQ(1U, decoded.count);
    EXPECT_NEAR(12.3F, decoded.values[0], 0.001F);
}

// UT-21: unknown identifiers and NULL frames fail with decoded count cleared.
TEST(DecodeFrame, RejectsUnknownIdentifierAndNullFrame)
{
    CanFrame_t frame = engineFrame();
    DecodedValues_t decoded = {2U, {1.0F, 2.0F, 3.0F}};
    frame.id = 0x7FFU;
    EXPECT_EQ(STATUS_ERR_UNKNOWN_ID, decodeFrame(&frame, &decoded));
    EXPECT_EQ(0U, decoded.count);
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, decodeFrame(nullptr, &decoded));
}

//test_frame_store
static StoredFrame_t stored(uint16_t id, Status_t status = STATUS_SUCCESS)
{
    StoredFrame_t item = {};
    item.frame.id = id;
    item.frame.dlc = DATA_BYTES;
    item.status = status;
    return item;
}

// UT-22: one frame can be added and retrieved by index.
TEST(FrameStore, AddsAndGetsFrame)
{
    FrameStore_t store = {};
    StoredFrame_t item = stored(ID_ENGINE_DATA);
    ASSERT_EQ(STATUS_SUCCESS, initFrameStore(&store));
    EXPECT_EQ(STATUS_SUCCESS, addFrame(&store, &item));
    EXPECT_EQ(1U, getFrameCount(&store));
    ASSERT_NE(nullptr, getFrame(&store, 0U));
    EXPECT_EQ(ID_ENGINE_DATA, getFrame(&store, 0U)->frame.id);
    EXPECT_EQ(nullptr, getFrame(&store, 1U));
}

// UT-23: the store accepts its 1000th frame and rejects the next one.
TEST(FrameStore, EnforcesCapacity)
{
    FrameStore_t store = {};
    StoredFrame_t item = stored(ID_ENGINE_DATA);
    ASSERT_EQ(STATUS_SUCCESS, initFrameStore(&store));
    for (uint16_t i = 0U; i < (MAX_FRAMES - 1U); i++)
        ASSERT_EQ(STATUS_SUCCESS, addFrame(&store, &item));
    EXPECT_EQ(STATUS_SUCCESS, addFrame(&store, &item));
    EXPECT_EQ(STATUS_ERR_STORE_FULL, addFrame(&store, &item));
    EXPECT_EQ(MAX_FRAMES, getFrameCount(&store));
}

// UT-24: statistics count received, valid and erroneous frames by identifier.
TEST(FrameStore, ComputesIdentifierStatistics)
{
    FrameStore_t store = {};
    IdStatistics_t stats = {};
    StoredFrame_t good = stored(ID_ENGINE_DATA);
    StoredFrame_t bad = stored(ID_ENGINE_DATA, STATUS_ERR_BAD_CHECKSUM);
    StoredFrame_t speed = stored(ID_VEHICLE_SPEED);
    ASSERT_EQ(STATUS_SUCCESS, initFrameStore(&store));
    ASSERT_EQ(STATUS_SUCCESS, addFrame(&store, &good));
    ASSERT_EQ(STATUS_SUCCESS, addFrame(&store, &bad));
    ASSERT_EQ(STATUS_SUCCESS, addFrame(&store, &good));
    ASSERT_EQ(STATUS_SUCCESS, addFrame(&store, &speed));
    EXPECT_EQ(STATUS_SUCCESS, computeIdStatistics(&store, ID_ENGINE_DATA, &stats));
    EXPECT_EQ(3U, stats.received);
    EXPECT_EQ(2U, stats.valid);
    EXPECT_EQ(1U, stats.errors);
}

// UT-25: matching frame indices are returned in storage order.
TEST(FrameStore, FindsFramesByIdentifier)
{
    FrameStore_t store = {};
    StoredFrame_t first = stored(ID_ENGINE_DATA);
    StoredFrame_t other = stored(ID_VEHICLE_SPEED);
    uint16_t indices[3] = {};
    uint16_t count = 0U;
    ASSERT_EQ(STATUS_SUCCESS, initFrameStore(&store));
    ASSERT_EQ(STATUS_SUCCESS, addFrame(&store, &first));
    ASSERT_EQ(STATUS_SUCCESS, addFrame(&store, &other));
    ASSERT_EQ(STATUS_SUCCESS, addFrame(&store, &first));
    EXPECT_EQ(STATUS_SUCCESS, findFramesById(&store, ID_ENGINE_DATA, indices, 3U, &count));
    ASSERT_EQ(2U, count);
    EXPECT_EQ(0U, indices[0]);
    EXPECT_EQ(2U, indices[1]);
}

// UT-26: a small result array receives only the first matching indices.
TEST(FrameStore, LimitsIdentifierSearchResults)
{
    FrameStore_t store = {};
    StoredFrame_t item = stored(ID_ENGINE_DATA);
    uint16_t indices[2] = {};
    uint16_t count = 0U;
    ASSERT_EQ(STATUS_SUCCESS, initFrameStore(&store));
    for (uint16_t i = 0U; i < 3U; i++) ASSERT_EQ(STATUS_SUCCESS, addFrame(&store, &item));
    EXPECT_EQ(STATUS_SUCCESS, findFramesById(&store, ID_ENGINE_DATA, indices, 2U, &count));
    EXPECT_EQ(2U, count);
    EXPECT_EQ(0U, indices[0]);
    EXPECT_EQ(1U, indices[1]);
}

// UT-27: error-frame search returns only items with a failing status.
TEST(FrameStore, FindsErrorFrames)
{
    FrameStore_t store = {};
    StoredFrame_t good = stored(ID_ENGINE_DATA);
    StoredFrame_t bad = stored(ID_ENGINE_DATA, STATUS_ERR_BAD_CHECKSUM);
    uint16_t indices[4] = {};
    uint16_t count = 0U;
    ASSERT_EQ(STATUS_SUCCESS, initFrameStore(&store));
    ASSERT_EQ(STATUS_SUCCESS, addFrame(&store, &good));
    ASSERT_EQ(STATUS_SUCCESS, addFrame(&store, &bad));
    ASSERT_EQ(STATUS_SUCCESS, addFrame(&store, &good));
    ASSERT_EQ(STATUS_SUCCESS, addFrame(&store, &bad));
    EXPECT_EQ(STATUS_SUCCESS, findErrorFrames(&store, indices, 4U, &count));
    EXPECT_EQ(2U, count);
    EXPECT_EQ(1U, indices[0]);
    EXPECT_EQ(3U, indices[1]);
}

//test_menu
// UT-28: menu choices zero and six and trimmed whitespace are accepted.
TEST(ParseMenuChoice, AcceptsBoundaryChoicesAndWhitespace)
{
    EXPECT_EQ(MENU_EXIT, parseMenuChoice("0"));
    EXPECT_EQ(MENU_SINGLE_FRAME, parseMenuChoice("6"));
    EXPECT_EQ(MENU_STATISTICS, parseMenuChoice("  3\n"));
    EXPECT_EQ(MENU_INVALID, parseMenuChoice("7"));
}

// UT-29: nonnumeric, empty, multiple-character and NULL inputs are invalid.
TEST(ParseMenuChoice, RejectsMalformedInput)
{
    EXPECT_EQ(MENU_INVALID, parseMenuChoice("abc"));
    EXPECT_EQ(MENU_INVALID, parseMenuChoice(""));
    EXPECT_EQ(MENU_INVALID, parseMenuChoice("12"));
    EXPECT_EQ(MENU_INVALID, parseMenuChoice("-1"));
    EXPECT_EQ(MENU_INVALID, parseMenuChoice(nullptr));
}

//test_report
// UT-30: every defined status has unique readable text and unknown values are safe.
TEST(StatusToText, CoversEveryStatus)
{
    std::set<std::string> descriptions;
    for (int value = STATUS_SUCCESS; value <= STATUS_ERR_BAD_CHECKSUM; value++)
    {
        const char *description = statusToText(static_cast<Status_t>(value));
        ASSERT_NE(nullptr, description);
        EXPECT_STRNE("", description);
        descriptions.insert(description);
    }
    EXPECT_EQ(15U, descriptions.size());
    EXPECT_STREQ("Unknown status", statusToText(static_cast<Status_t>(999)));
}