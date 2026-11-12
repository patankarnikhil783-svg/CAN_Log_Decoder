/*
validation.c
Checks CAN identifiers, data length and the project checksum.
*/

#include <stddef.h>

#include "validation.h"
#include "decoder.h"

// Calculates the XOR checksum of data bytes zero through six.
Status_t calculateChecksum(const uint8_t *data, uint8_t *checksum)
{
    uint8_t result = 0U;

    uint8_t i;

    if ((data == NULL) || (checksum == NULL)) 
    {
        return STATUS_ERR_NULL_POINTER;
    }

    for (i = 0U; i < CHECKSUM_INDEX; i++) 
    {
        result = (uint8_t)(result ^ data[i]);
    }

    *checksum = result;
    return STATUS_SUCCESS;
}

// Checks known identifier, eight-byte length, then checksum in design order.
Status_t validateFrame(const CanFrame_t *frame)
{
    uint8_t expected = 0U;
    Status_t status;
    if (frame == NULL) 
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (findMessageDefinition(frame->id) == NULL) 
    {
        return STATUS_ERR_UNKNOWN_ID;
    }
    
    if (frame->dlc != DATA_BYTES) {
        return STATUS_ERR_BAD_LENGTH;
    }

    status = calculateChecksum(frame->data, &expected);

    if (status != STATUS_SUCCESS) 
    {
        return status;
    }
    if (expected != frame->data[CHECKSUM_INDEX])
    {
        return STATUS_ERR_BAD_CHECKSUM;
    } 
    
    return STATUS_SUCCESS;
}