/*
file_handler.c
Owns opening, bounded line reading and closing of CAN log files.
*/

#include <stdio.h>
#include <string.h>

#include "file_handler.h"

// Reads the header line and discards any part of it that does not fit in the buffer.
static Status_t consumeHeaderLine(FILE *file)
{
    char header[MAX_LINE_LENGTH];
    size_t headerLength;
    int character;

    if (fgets(header, (int)sizeof(header), file) == NULL)
    {
        return ferror(file) ? STATUS_ERR_FILE_READ : STATUS_ERR_FILE_EMPTY;
    }
    headerLength = strlen(header);

    if ((headerLength > 0U) && (header[headerLength - 1U] != '\n') && !feof(file))
    {
        do
        {
            character = fgetc(file);
        } while ((character != '\n') && (character != EOF));

        if (ferror(file))
        {
            return STATUS_ERR_FILE_READ;
        }
    }
    return STATUS_SUCCESS;
}

// Opens a log and consumes its header line.
Status_t openLogFile(const char *fileName, FILE **file)
{
    FILE *opened;
    Status_t status;

    if (file == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }
    *file = NULL;

    if (fileName == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (fileName[0] == '\0')
    {
        return STATUS_ERR_INVALID_ARGUMENT;
    }

    opened = fopen(fileName, "r");
    if (opened == NULL)
    {
        return STATUS_ERR_FILE_OPEN;
    }

    status = consumeHeaderLine(opened);
    if (status != STATUS_SUCCESS)
    {
        (void)fclose(opened);
        return status;
    }
    *file = opened;
    return STATUS_SUCCESS;
}

// Handles the end of a line that filled the buffer: swallows a CR/LF pair, or drains the rest
// of an over-long line and reports it.
static Status_t finishLine(FILE *file, char *buffer)
{
    int next = fgetc(file);

    if ((next != '\n') && (next != '\r') && (next != EOF))
    {
        while ((next != '\n') && (next != EOF))
        {
            next = fgetc(file);
        }
        buffer[0] = '\0';
        return ferror(file) ? STATUS_ERR_FILE_READ : STATUS_ERR_LINE_TOO_LONG;
    }
    if (next == '\r')
    {
        next = fgetc(file);
        if ((next != '\n') && (next != EOF))
        {
            (void)ungetc(next, file);
        }
    }
    return STATUS_SUCCESS;
}

// Reads one bounded line, removes CR/LF and drains over-long input lines.
Status_t readLogLine(FILE *file, char *buffer, size_t size)
{
    size_t length;
    Status_t status = STATUS_SUCCESS;

    if ((file == NULL) || (buffer == NULL))
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (size < 2U)
    {
        return STATUS_ERR_INVALID_ARGUMENT;
    }

    if (fgets(buffer, (int)size, file) == NULL)
    {
        if (ferror(file))
        {
            return STATUS_ERR_FILE_READ;
        }
        return STATUS_END_OF_FILE;
    }

    length = 0U;
    while ((buffer[length] != '\0') && (buffer[length] != '\n') && (buffer[length] != '\r'))
    {
        length++;
    }

    if ((buffer[length] == '\0') && !feof(file))
    {
        status = finishLine(file, buffer);
    }
    if (status == STATUS_SUCCESS)
    {
        buffer[length] = '\0';
    }
    return status;
}

// Closes an open file; NULL is accepted.
void closeLogFile(FILE *file)
{
    if (file != NULL)
    {
        (void)fclose(file);
    }
}