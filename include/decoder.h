/* CAN message definitions and signal decoding. */
#ifndef DECODER_H
#define DECODER_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

const MessageDefinition_t *findMessageDefinition(uint16_t id);

Status_t extractBits(const uint8_t *data, uint8_t startBit, uint8_t bitLength, uint32_t *raw);

float convertRawValue(uint32_t raw, float factor, float offset);

Status_t decodeFrame(const CanFrame_t *frame, DecodedValues_t *decoded);
#ifdef __cplusplus

}
#endif

#endif // DECODER_H