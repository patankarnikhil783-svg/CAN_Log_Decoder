/*
report.c
Formats frame results, errors, statistics and summary output.
*/
#include <inttypes.h>
#include <stdio.h>

#include "report.h"
#include "decoder.h"
#include "validation.h"
#include "frame_store.h"

// Converts a status code into the user-facing description in the design. 
const char *statusToText(Status_t status)
{
    switch (status)
    {
        case STATUS_SUCCESS: return "Success";
        case STATUS_END_OF_FILE: return "End of file";
        case STATUS_ERR_NULL_POINTER: return "Internal error: null pointer";
        case STATUS_ERR_INVALID_ARGUMENT: return "Invalid argument";
        case STATUS_ERR_FILE_OPEN: return "Cannot open the file";
        case STATUS_ERR_FILE_EMPTY: return "The file has no header line";
        case STATUS_ERR_FILE_READ: return "Error while reading the file";
        case STATUS_ERR_LINE_TOO_LONG: return "Line is too long";
        case STATUS_ERR_FIELD_COUNT: return "Wrong number of fields (expected 11)";
        case STATUS_ERR_INVALID_HEX: return "Invalid number";
        case STATUS_ERR_OUT_OF_RANGE: return "Value out of range";
        case STATUS_ERR_STORE_FULL: return "Frame store is full";
        case STATUS_ERR_UNKNOWN_ID: return "Unknown identifier";
        case STATUS_ERR_BAD_LENGTH: return "Data length is not 8";
        case STATUS_ERR_BAD_CHECKSUM: return "Checksum mismatch";
        default: return "Unknown status";
    }
}

// Prints one frame with decoded signal values or its validation error.
void printFrameResult(FILE *out, const StoredFrame_t *item)
{
    const MessageDefinition_t *definition;
    uint8_t i;
    uint8_t expected = 0U;

    if ((out == NULL) || (item == NULL)) 
    {
        return;
    }
    (void)fprintf(out, "Line %" PRIu32 " | time=%" PRIu32 " ms | ID=0x%03X | DLC=%u\n",
                  item->lineNumber, item->frame.timeMs, item->frame.id, item->frame.dlc);

    if (item->status != STATUS_SUCCESS)
    {
        (void)fprintf(out, "  ERROR: %s", statusToText(item->status));
        if ((item->status == STATUS_ERR_BAD_CHECKSUM) &&
            (calculateChecksum(item->frame.data, &expected) == STATUS_SUCCESS))
        {
            (void)fprintf(out, " (expected %02X, received %02X)", expected,
                          item->frame.data[CHECKSUM_INDEX]);
        }
        (void)fprintf(out, "\n");
        return;
    }
    definition = findMessageDefinition(item->frame.id);
    if (definition == NULL) return;
    for (i = 0U; i < item->decoded.count; i++)
    {
        (void)fprintf(out, "  %-22s = %.3f %s\n", definition->signals[i].name,
                      (double)item->decoded.values[i], definition->signals[i].unit);
    }
}

// Prints the reason a malformed source line was rejected. 
void printRejectedLine(FILE *out, uint32_t lineNumber, Status_t status)
{
    if (out == NULL) 
    {
        return;
    }
    (void)fprintf(out, "Line %" PRIu32 " rejected: %s\n", lineNumber, statusToText(status));
}

// Prints frames selected by their indices in the store.
void printFrameList(FILE *out, const FrameStore_t *store, const uint16_t *indices, uint16_t count)
{
    uint16_t i;
    const StoredFrame_t *item;

    if ((out == NULL) || (store == NULL) || (indices == NULL)) 
    {
        return;
    }

    if (count == 0U) 
    {
        (void)fprintf(out, "No matching frames.\n");
    }

    for (i = 0U; i < count; i++)
    {
        item = getFrame(store, indices[i]);

        if (item != NULL) 
        {
            printFrameResult(out, item);
        }
    }
}

// Prints statistics for the three known IDs and all unknown identifiers.
void printStatistics(FILE *out, const FrameStore_t *store)
{
    static const uint16_t knownIds[MESSAGE_COUNT] =
        {ID_ENGINE_DATA, ID_VEHICLE_SPEED, ID_BATTERY_STATUS};
    
    IdStatistics_t stats;
    uint32_t unknown = 0U;
    uint16_t i;

    if ((out == NULL) || (store == NULL)) 
    {
        return;
    }
    (void)fprintf(out, "\nIdentifier statistics\n");

    for (i = 0U; i < MESSAGE_COUNT; i++)
    {
        if (computeIdStatistics(store, knownIds[i], &stats) == STATUS_SUCCESS)
        {
            (void)fprintf(out, "ID 0x%03X: received=%" PRIu32 ", valid=%" PRIu32
                          ", errors=%" PRIu32 "\n", stats.id, stats.received,
                          stats.valid, stats.errors);
        }
    }

    for (i = 0U; i < store->count; i++)
    {
        if (findMessageDefinition(store->items[i].frame.id) == NULL) 
        {
            unknown++;
        }
    }
    (void)fprintf(out, "Unknown identifiers: %" PRIu32 "\n", unknown);
}

// Prints the processing totals collected during the last decode operation.
void printSummary(FILE *out, const Summary_t *summary)
{
    if ((out == NULL) || (summary == NULL)) 
    {
        return;
    }
    
    (void)fprintf(out, "\n========== DECODE SUMMARY ==========\n");
    (void)fprintf(out, "Lines read:          %" PRIu32 "\n", summary->linesRead);
    (void)fprintf(out, "Lines rejected:      %" PRIu32 "\n", summary->linesRejected);
    (void)fprintf(out, "Frames decoded:      %" PRIu32 "\n", summary->framesDecoded);
    (void)fprintf(out, "Unknown ID errors:   %" PRIu32 "\n", summary->unknownIdErrors);
    (void)fprintf(out, "Length errors:       %" PRIu32 "\n", summary->lengthErrors);
    (void)fprintf(out, "Checksum errors:     %" PRIu32 "\n", summary->checksumErrors);
}