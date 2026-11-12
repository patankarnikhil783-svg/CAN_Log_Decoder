/* Frame integrity checks. */
#ifndef VALIDATION_H
#define VALIDATION_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

Status_t calculateChecksum(const uint8_t *data, uint8_t *checksum);

Status_t validateFrame(const CanFrame_t *frame);

#ifdef __cplusplus
}
#endif
#endif // VALIDATION_H