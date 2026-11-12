/*
file_handler.c
Owns opening, bounded line reading and closing of CAN log files.
*/

#include <stdio.h>
#include <string.h>

#include "file_handler.h"

// Opens a log and consumes its header line.
Status_t openLogFile(const char *fileName, FILE **file)
{
    char header[MAX_LINE_LENGTH];
    FILE *opened;
    size_t headerLength;

    if (file == NULL) 
    {
        return STATUS_ERR_NULL_POINTER;
    }
    *file = NULL;

    if (fileName == NULL) 
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (fileName[0] == '\0') r
    {
        return STATUS_ERR_INVALID_ARGUMENT;
    }

    opened = fopen(fileName, "r");
    if (opened == NULL) 
    {
        return STATUS_ERR_FILE_OPEN;
    }

    if (fgets(header, (int)sizeof(header), opened) == NULL)
    {
        Status_t status = ferror(opened) ? STATUS_ERR_FILE_READ : STATUS_ERR_FILE_EMPTY;
        (void)fclose(opened);
        return status;
    }
    headerLength = strlen(header);

    if ((headerLength > 0U) && (header[headerLength - 1U] != '\n') && !feof(opened))
    {
        int character;
        do
        {
            character = fgetc(opened);
        } while ((character != '\n') && (character != EOF));

        if (ferror(opened))
        {
            (void)fclose(opened);
            return STATUS_ERR_FILE_READ;
        }
    }
    *file = opened;
    return STATUS_SUCCESS;
}

// Reads one bounded line, removes CR/LF and drains over-long input lines. 
Status_t readLogLine(FILE *file, char *buffer, size_t size)
{
    size_t length;
    int next;
    if ((file == NULL) || (buffer == NULL)) {
        return STATUS_ERR_NULL_POINTER;
    }

    if (size < 2U) {
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
        next = fgetc(file);
        if ((next != '\n') && (next != '\r') && (next != EOF))
        {
            while ((next != '\n') && (next != EOF)) next = fgetc(file);
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
    }
    buffer[length] = '\0';
    return STATUS_SUCCESS;
}

// Closes an open file; NULL is accepted.
void closeLogFile(FILE *file)
{
    if (file != NULL) (void)fclose(file);
}