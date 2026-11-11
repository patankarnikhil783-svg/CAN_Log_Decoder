/*
parser.c
Converts text into numbers and one CSV line into a CAN frame.
*/

#include <stddef.h>

#include "parser.h"

// Returns the value 0 to 15 of a hexadecimal digit, or -1 if c is not one.
static int hexDigitValue(char c)
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

// Converts hexadecimal text (optional 0x prefix) to an unsigned number.
Status_t parseHexValue(const char *text, uint32_t maxValue, uint32_t *value)
{
    const char *position = text;
    uint64_t result = 0;
    int tooLarge = 0;
    int digit = 0;

    if (text == NULL || value == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (position[0] == '0' && (position[1] == 'x' || position[1] == 'X'))
    {
        position += 2;
    }

    if (*position == '\0')
    {
        return STATUS_ERR_INVALID_HEX;
    }

    for (; *position != '\0'; position++)
    {
        digit = hexDigitValue(*position);
        if (digit < 0)
        {
            return STATUS_ERR_INVALID_HEX;
        }

        if (!tooLarge)
        {
            result = (result << 4) | (uint64_t)digit;
            if (result > maxValue)
            {
                tooLarge = 1;
            }
        }
    }

    if (tooLarge)
    {
        return STATUS_ERR_OUT_OF_RANGE;
    }

    *value = (uint32_t)result;
    return STATUS_SUCCESS;
}