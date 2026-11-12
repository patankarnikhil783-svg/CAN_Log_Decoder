// Console report formatting.
#ifndef REPORT_H
#define REPORT_H

#include <stdio.h>

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *statusToText(Status_t status);

void printFrameResult(FILE *out, const StoredFrame_t *item);

void printRejectedLine(FILE *out, uint32_t lineNumber, Status_t status);

void printFrameList(FILE *out, const FrameStore_t *store, const uint16_t *indices, uint16_t count);

void printStatistics(FILE *out, const FrameStore_t *store);

void printSummary(FILE *out, const Summary_t *summary);

#ifdef __cplusplus
}
#endif

#endif // REPORT_H