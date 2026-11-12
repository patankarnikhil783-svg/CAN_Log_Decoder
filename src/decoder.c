/*
 * decoder.c
 *
 * Decodes fixed CAN message definitions into physical signal values.
 */
#include <stddef.h>
#include <stdint.h>

#include "decoder.h"

static const MessageDefinition_t messageTable[MESSAGE_COUNT] =
{
    { ID_ENGINE_DATA, "EngineData", 3U,
      { {"engine speed", "rpm", 0U, 16U, 0.25F, 0.0F},

        {"coolant temperature", "deg C", 16U, 8U, 1.0F, -40.0F},

        {"throttle position", "%", 24U, 8U, 0.4F, 0.0F} } },

    { ID_VEHICLE_SPEED, "VehicleSpeed", 3U,
      { {"vehicle speed", "km/h", 0U, 16U, 0.01F, 0.0F},

        {"brake pressed", "", 16U, 1U, 1.0F, 0.0F},

        {"gear", "", 17U, 3U, 1.0F, 0.0F} } },

    { ID_BATTERY_STATUS, "BatteryStatus", 1U,
      { {"battery voltage", "V", 0U, 8U, 0.1F, 0.0F} } }
};

// Returns the constant message definition for an identifier, or NULL if unknown.
const MessageDefinition_t *findMessageDefinition(uint16_t id)
{
    uint8_t i;

    for (i = 0U; i < MESSAGE_COUNT; i++)
    {
        if (messageTable[i].id == id)
        {
            return &messageTable[i];
        }
    }
    return NULL;
}

// Extracts an unsigned little-endian bit field without modifying output on error.
Status_t extractBits(const uint8_t *data, uint8_t startBit, uint8_t bitLength, uint32_t *raw)
{
    uint64_t packed = 0U;
    uint32_t mask;
    uint8_t i;

    if ((data == NULL) || (raw == NULL))
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if ((bitLength == 0U) || (bitLength > 32U) ||
        (((uint16_t)startBit + (uint16_t)bitLength) > 64U))
    {
        return STATUS_ERR_INVALID_ARGUMENT;
    }

    for (i = 0U; i < DATA_BYTES; i++)
    {
        packed |= ((uint64_t)data[i] << ((uint32_t)i * 8U));
    }

    if (bitLength == 32U)
    {
        mask = UINT32_MAX;
    }
    else
    {
        mask = (uint32_t)((1UL << bitLength) - 1UL);
    }

    *raw = (uint32_t)((packed >> startBit) & (uint64_t)mask);

    return STATUS_SUCCESS;
}

// Applies the physical conversion formula raw multiplied by factor plus offset.
float convertRawValue(uint32_t raw, float factor, float offset)
{
    return ((float)raw * factor) + offset;
}

// Decodes every signal for a known message; decoded count is zero on failure.
Status_t decodeFrame(const CanFrame_t *frame, DecodedValues_t *decoded)
{
    const MessageDefinition_t *definition;

    DecodedValues_t result = {0U, {0.0F, 0.0F, 0.0F}};

    uint32_t raw = 0U;
    uint8_t i;
    Status_t status;

    if (decoded == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }
    decoded->count = 0U;

    if (frame == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }
    definition = findMessageDefinition(frame->id);

    if (definition == NULL)
    {
        return STATUS_ERR_UNKNOWN_ID;
    }

    for (i = 0U; i < definition->signalCount; i++)
    {
        status = extractBits(frame->data, definition->signals[i].startBit,
                             definition->signals[i].bitLength, &raw);

        if (status != STATUS_SUCCESS)
        {
            return status;
        }
        result.values[i] = convertRawValue(raw, definition->signals[i].factor,
                                            definition->signals[i].offset);
    }

    result.count = definition->signalCount;
    *decoded = result;

    return STATUS_SUCCESS;
}