//Converts text into numbers and one CSV line into a CAN frame.

#include <stddef.h>
#include <string.h>

#include "parser.h"

#define MAX_BYTE_VALUE 0xFFu // largest value of one data byte           
#define FIELD_BUFFER_SIZE 16 // room for one field of text plus the NUL  

// Positions of the fields in a data line.
#define FIELD_TIME 0
#define FIELD_ID 1
#define FIELD_DLC 2
#define FIELD_FIRST_BYTE 3

// Returns the value 0 to 15 of a hexadecimal digit, or -1 if c is not one. 
static int digitValue(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }
    return -1;
}

// Converts digits in the given base (10 or 16) to a number no larger than maxValue.
static Status_t parseNumber(const char *digits, uint32_t base, uint32_t maxValue,
                            uint32_t *value)
{
    uint64_t result = 0;
    int tooLarge = 0;
    int digit = 0;

    if (*digits == '\0')
    {
        return STATUS_ERR_INVALID_HEX;
    }

    for (; *digits != '\0'; digits++)
    {
        digit = digitValue(*digits);
        if (digit < 0 || (uint32_t)digit >= base)
        {
            return STATUS_ERR_INVALID_HEX;
        }
        if (!tooLarge)
        {
            result = (result * base) + (uint64_t)digit;
            tooLarge = (result > maxValue);
        }
    }

    if (tooLarge)
    {
        return STATUS_ERR_OUT_OF_RANGE;
    }
    *value = (uint32_t)result;
    return STATUS_SUCCESS;
}

// Converts decimal text to an unsigned number no larger than maxValue.
Status_t parseDecimalValue(const char *text, uint32_t maxValue, uint32_t *value)
{
    if (text == NULL || value == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }
    return parseNumber(text, 10, maxValue, value);
}

// Converts hexadecimal text (optional 0x prefix) to a number no larger than maxValue.
Status_t parseHexValue(const char *text, uint32_t maxValue, uint32_t *value)
{
    if (text == NULL || value == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }
    if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
    {
        text += 2;
    }
    return parseNumber(text, 16, maxValue, value);
}

// Returns 1 if the text is empty or only white space, otherwise 0.
int isBlankLine(const char *line)
{
    if (line == NULL)
    {
        return 1;
    }
    for (; *line != '\0'; line++)
    {
        if (*line != ' ' && *line != '\t' && *line != '\r' && *line != '\n')
        {
            return 0;
        }
    }
    return 1;
}

/*
 Copies the field that begins at *cursor into buffer and moves *cursor to the start
 of the next field. The field ends at a comma or at end. Fails if it does not fit.
 */
static Status_t readField(const char **cursor, const char *end, char *buffer, size_t size)
{
    const char *position = *cursor;
    size_t length = 0;

    while (position < end && *position != ',')
    {
        if (length + 1 >= size)
        {
            return STATUS_ERR_INVALID_HEX;
        }
        buffer[length++] = *position++;
    }
    buffer[length] = '\0';
    *cursor = (position < end) ? position + 1 : position;
    return STATUS_SUCCESS;
}

// Converts one field, chosen by its index, and stores it in the frame. 
static Status_t convertField(size_t index, const char *field, CanFrame_t *frame)
{
    uint32_t number = 0;
    Status_t status = STATUS_SUCCESS;

    if (index == FIELD_TIME)
    {
        status = parseDecimalValue(field, UINT32_MAX, &number);
        frame->timeMs = number;
    }
    else if (index == FIELD_ID)
    {
        status = parseHexValue(field, MAX_CAN_ID, &number);
        frame->id = (uint16_t)number;
    }
    else if (index == FIELD_DLC)
    {
        status = parseDecimalValue(field, DATA_BYTES, &number);
        frame->dlc = (uint8_t)number;
    }
    else
    {
        status = parseHexValue(field, MAX_BYTE_VALUE, &number);
        frame->data[index - FIELD_FIRST_BYTE] = (uint8_t)number;
    }
    return status;
}


// Parses one data line (11 comma-separated fields, line ending ignored) into a frame.

Status_t parseFrameLine(const char *line, CanFrame_t *frame)
{
    const char *end = NULL;
    const char *cursor = NULL;
    char field[FIELD_BUFFER_SIZE];
    CanFrame_t result = {0};
    Status_t status = STATUS_SUCCESS;
    size_t commas = 0;
    size_t index = 0;

    if (line == NULL || frame == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    end = line + strlen(line);
    while (end > line && (end[-1] == '\r' || end[-1] == '\n'))
    {
        end--;
    }

    for (cursor = line; cursor < end; cursor++)
    {
        commas += (*cursor == ',') ? 1 : 0;
    }
    if (commas + 1 != FIELDS_PER_LINE)
    {
        return STATUS_ERR_FIELD_COUNT;
    }

    cursor = line;
    for (index = 0; index < FIELDS_PER_LINE && status == STATUS_SUCCESS; index++)
    {
        status = readField(&cursor, end, field, sizeof(field));
        if (status == STATUS_SUCCESS)
        {
            status = convertField(index, field, &result);
        }
    }
    if (status == STATUS_SUCCESS)
    {
        *frame = result;
    }
    return status;
}
