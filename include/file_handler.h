// File input operations; this is the only module that accesses log files.
#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <stdio.h>
#include <stddef.h>

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

Status_t openLogFile(const char *fileName, FILE **file);

Status_t readLogLine(FILE *file, char *buffer, size_t size);

void closeLogFile(FILE *file);

#ifdef __cplusplus
}
#endif

#endif // FILE_HANDLER_H