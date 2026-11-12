/*
menu.c
Handles all keyboard input using bounded line buffers.
*/
#include <stdio.h>
#include <string.h>

#include "menu.h"
#include "parser.h"

static Status_t readInputLine(char *buffer, size_t size);

// Reads one bounded line and discards any excess characters.
static Status_t readInputLine(char *buffer, size_t size)
{
    size_t length;
    int ch;
    if ((buffer == NULL) || (size < 2U)) 
    {
        return STATUS_ERR_INVALID_ARGUMENT;
    }

    if (fgets(buffer, (int)size, stdin) == NULL) 
    {
        return STATUS_END_OF_FILE;
    }
    length = strlen(buffer);
    
    if ((length > 0U) && (buffer[length - 1U] == '\n'))
    {
        buffer[length - 1U] = '\0';
        if ((length > 1U) && (buffer[length - 2U] == '\r')) 
        {
            buffer[length - 2U] = '\0';
        }
        return STATUS_SUCCESS;
    }

    if (!feof(stdin))
    {
        ch = getchar();
        if ((ch != '\n') && (ch != EOF))
        {
            while ((ch != '\n') && (ch != EOF)) 
            {
                ch = getchar();
            }
            buffer[0] = '\0';
            return STATUS_ERR_LINE_TOO_LONG;
        }
    }
    return STATUS_SUCCESS;
}

// Converts a trimmed single digit from zero through six into a menu choice.
MenuChoice_t parseMenuChoice(const char *text)
{
    const char *p;
    char choice;
    if (text == NULL) 
    {
        return MENU_INVALID;
    }
    p = text;
    while ((*p == ' ') || (*p == '\t') || (*p == '\r') || (*p == '\n')) 
    {
        p++;
    }

    choice = *p;
    if ((choice < '0') || (choice > '6')) 
    {
        return MENU_INVALID;
    }
    p++;

    while ((*p == ' ') || (*p == '\t') || (*p == '\r') || (*p == '\n')) 
    {
        p++;
    }

    if (*p != '\0') 
    {
        return MENU_INVALID;
    }
    return (MenuChoice_t)(choice - '0');
}

// Displays the main menu and reads one validated choice. 
MenuChoice_t readMenuChoice(void)
{
    char input[MAX_INPUT_LENGTH];
    Status_t status;

    (void)printf("\n========== CAN LOG DECODER ==========\n");
    (void)printf("1. Load log file\n2. Decode loaded file\n3. Show statistics\n");
    (void)printf("4. Show frames by identifier\n5. Show error frames\n");
    (void)printf("6. Decode a single frame\n0. Exit\nChoice: ");

    status = readInputLine(input, sizeof(input));

    if (status == STATUS_END_OF_FILE) 
    {
        return MENU_EXIT;
    }

    if (status != STATUS_SUCCESS) 
    {
        return MENU_INVALID;
    }
    return parseMenuChoice(input);
}

// Reads a non-empty file name into a caller-provided bounded buffer.
Status_t readFileName(char *buffer, size_t size)
{
    Status_t status;
    size_t start = 0U;
    size_t end;

    if ((buffer == NULL) || (size < 2U)) 
    {
        return STATUS_ERR_NULL_POINTER;
    }

    (void)printf("Enter log file name: ");
    status = readInputLine(buffer, size);

    if (status != STATUS_SUCCESS) 
    {
        return status;
    }

    while ((buffer[start] == ' ') || (buffer[start] == '\t')) 
    {
        start++;
    }
    end = strlen(buffer);

    while ((end > start) && ((buffer[end - 1U] == ' ') || (buffer[end - 1U] == '\t'))) 
    {
        end--;
    }

    if (start > 0U) 
    {
        memmove(buffer, &buffer[start], end - start);
    }
    buffer[end - start] = '\0';
    return (buffer[0] == '\0') ? STATUS_ERR_INVALID_ARGUMENT : STATUS_SUCCESS;
}

// Reads a hexadecimal identifier no larger than the standard 11-bit maximum. 
Status_t readIdentifier(uint16_t *id)
{
    char input[MAX_INPUT_LENGTH];
    uint32_t parsed = 0U;
    Status_t status;

    if (id == NULL) 
    {
        return STATUS_ERR_NULL_POINTER;
    }
    (void)printf("Enter identifier (hex): ");
    status = readInputLine(input, sizeof(input));
    if (status != STATUS_SUCCESS) return status;
    status = parseHexValue(input, MAX_CAN_ID, &parsed);
    if (status == STATUS_SUCCESS) *id = (uint16_t)parsed;
    return status;
}

// Reads an identifier, length and eight data bytes into a frame.
Status_t readFrameFromUser(CanFrame_t *frame)
{
    CanFrame_t result = {0U, 0U, 0U, {0U}};
    char input[MAX_INPUT_LENGTH];
    uint32_t number = 0U;
    uint8_t i;
    Status_t status;
    if (frame == NULL) 
    {
        return STATUS_ERR_NULL_POINTER;
    }
    status = readIdentifier(&result.id);

    if (status != STATUS_SUCCESS) 
    {
        return status;
    }
    (void)printf("Enter data length (0-8): ");
    
    status = readInputLine(input, sizeof(input));
    if (status != STATUS_SUCCESS) 
    {
        return status;
    }
    status = parseDecimalValue(input, DATA_BYTES, &number);
    
    if (status != STATUS_SUCCESS) 
    {
        return status;
    }
    result.dlc = (uint8_t)number;
    
    for (i = 0U; i < DATA_BYTES; i++)
    {
        (void)printf("Enter data byte %u (hex): ", (unsigned int)i);
        status = readInputLine(input, sizeof(input));
        
        if (status != STATUS_SUCCESS) return status;
        if (strlen(input) > 2U) 
        {
            return STATUS_ERR_INVALID_HEX;
        }
        status = parseHexValue(input, 0xFFU, &number);
        
        if (status != STATUS_SUCCESS) 
        {
            return status;
        }
        result.data[i] = (uint8_t)number;
    }
    *frame = result;
    return STATUS_SUCCESS;
}