/*
parser.h
Converts text into numbers and one CSV line into a CAN frame.
No input or output is performed in this module.
*/

#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

//Converts decimal text to an unsigned number no larger than maxValue.
Status_t parseDecimalValue(const char *text, uint32_t maxValue, uint32_t *value);

/* Converts hexadecimal text (optional 0x prefix) to an unsigned number
 * no larger than maxValue. */
Status_t parseHexValue(const char *text, uint32_t maxValue, uint32_t *value);

// Parses one data line of the log (11 comma-separated fields) into a frame. 
Status_t parseFrameLine(const char *line, CanFrame_t *frame);

// Returns 1 if the text is empty or only white space, otherwise 0. 
int isBlankLine(const char *line);

#ifdef __cplusplus
}
#endif

#endif // PARSER_H 
