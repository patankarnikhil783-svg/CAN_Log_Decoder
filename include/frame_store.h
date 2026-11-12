/* Fixed-capacity storage for processed frames. */
#ifndef FRAME_STORE_H
#define FRAME_STORE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

Status_t initFrameStore(FrameStore_t *store);

Status_t addFrame(FrameStore_t *store, const StoredFrame_t *item);

const StoredFrame_t *getFrame(const FrameStore_t *store, uint16_t index);

uint16_t getFrameCount(const FrameStore_t *store);

Status_t computeIdStatistics(const FrameStore_t *store, uint16_t id, IdStatistics_t *stats);

Status_t findFramesById(const FrameStore_t *store, uint16_t id, uint16_t *indices,
                        uint16_t maxIndices, uint16_t *count);

Status_t findErrorFrames(const FrameStore_t *store, uint16_t *indices,
                         uint16_t maxIndices, uint16_t *count);

#ifdef __cplusplus
}
#endif
#endif // FRAME_STORE_H