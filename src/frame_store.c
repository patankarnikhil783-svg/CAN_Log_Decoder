/*
frame_store.c
Stores processed frames in a fixed-size array and provides queries.
*/

#include <stddef.h>

#include "frame_store.h"

// Initialises a frame store as empty.
Status_t initFrameStore(FrameStore_t *store)
{
    if (store == NULL) return STATUS_ERR_NULL_POINTER;
    store->count = 0U;
    return STATUS_SUCCESS;
}

// Appends one item; the store is unchanged if an error occurs.
Status_t addFrame(FrameStore_t *store, const StoredFrame_t *item)
{
    if ((store == NULL) || (item == NULL)) 
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (store->count >= MAX_FRAMES) 
    {
        return STATUS_ERR_STORE_FULL;
    }

    store->items[store->count] = *item;
    store->count++;

    return STATUS_SUCCESS;
}

// Returns a pointer to a stored item, or NULL for an invalid index.
const StoredFrame_t *getFrame(const FrameStore_t *store, uint16_t index)
{
    if ((store == NULL) || (index >= store->count)) 
    {
        return NULL;
    }
    return &store->items[index];
}

// Returns the number of stored frames, or zero for a NULL store.
uint16_t getFrameCount(const FrameStore_t *store)
{
    return (store == NULL) ? 0U : store->count;
}

// Counts received, valid and erroneous frames for one identifier.
Status_t computeIdStatistics(const FrameStore_t *store, uint16_t id, IdStatistics_t *stats)
{
    uint16_t i;
    IdStatistics_t result = {id, 0U, 0U, 0U};

    if ((store == NULL) || (stats == NULL)) 
    {
        return STATUS_ERR_NULL_POINTER;
    }

    for (i = 0U; i < store->count; i++)
    {
        if (store->items[i].frame.id == id)
        {
            result.received++;
            if (store->items[i].status == STATUS_SUCCESS) 
            {
                result.valid++;
            }
        }
    }
    result.errors = result.received - result.valid;
    *stats = result;

    return STATUS_SUCCESS;
}

/* Finds matching identifiers and writes at most maxIndices indices. */
Status_t findFramesById(const FrameStore_t *store, uint16_t id, uint16_t *indices,
                        uint16_t maxIndices, uint16_t *count)
{
    uint16_t i;
    uint16_t found = 0U;

    if ((store == NULL) || (indices == NULL) || (count == NULL)) 
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (maxIndices == 0U) 
    {
        return STATUS_ERR_INVALID_ARGUMENT;
    }

    for (i = 0U; (i < store->count) && (found < maxIndices); i++)
    {
        if (store->items[i].frame.id == id) indices[found++] = i;
    }

    *count = found;
    return STATUS_SUCCESS;
}

// Finds indices of frames whose validation status is not successful.
Status_t findErrorFrames(const FrameStore_t *store, uint16_t *indices,
                         uint16_t maxIndices, uint16_t *count)
{
    uint16_t i;
    uint16_t found = 0U;

    if ((store == NULL) || (indices == NULL) || (count == NULL)) 
    {
        return STATUS_ERR_NULL_POINTER;
    }
    if (maxIndices == 0U) 
    {
        return STATUS_ERR_INVALID_ARGUMENT;
    }
    for (i = 0U; (i < store->count) && (found < maxIndices); i++)
    {
        if (store->items[i].status != STATUS_SUCCESS) 
        {
            indices[found++] = i;
        }
    }
    
    *count = found;
    return STATUS_SUCCESS;
}