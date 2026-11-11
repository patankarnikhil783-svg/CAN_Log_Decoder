/*
* types.h
* Shared constants, enumerations and structures for the CAN Log Decoder.
* Every module includes this header. It contains no executable code.
*/

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


//---Constants---                                                          


#define MAX_FRAMES 1000 // frames kept in memory            
#define DATA_BYTES 8 // data bytes in every frame       
#define FIELDS_PER_LINE 11 // fields in one CSV data line      
#define CHECKSUM_INDEX 7 // index of the checksum byte       
#define MAX_CAN_ID 0x7FF // largest 11-bit identifier        
#define MESSAGE_COUNT 3 // number of known messages         
#define MAX_SIGNALS_PER_MESSAGE 3 // largest signal count of a message 
#define MAX_LINE_LENGTH 256 // buffer size for one text line    
#define MAX_FILE_NAME_LENGTH 128 // buffer size for a file name      
#define MAX_INPUT_ATTEMPTS 3 // retries after invalid input      
#define MAX_INPUT_LENGTH 16 // buffer size for a menu entry     

#define ID_ENGINE_DATA 0x100 //identifier of the engine message 
#define ID_VEHICLE_SPEED 0x200 // identifier of the speed message 
#define ID_BATTERY_STATUS 0x300 // identifier of the battery message


//---Enumerations---

// Result of any operation that can fail. Also stored with each frame.
typedef enum
{
    STATUS_SUCCESS = 0,
    STATUS_END_OF_FILE,
    STATUS_ERR_NULL_POINTER,
    STATUS_ERR_INVALID_ARGUMENT,
    STATUS_ERR_FILE_OPEN,
    STATUS_ERR_FILE_EMPTY,
    STATUS_ERR_FILE_READ,
    STATUS_ERR_LINE_TOO_LONG,
    STATUS_ERR_FIELD_COUNT,
    STATUS_ERR_INVALID_HEX,
    STATUS_ERR_OUT_OF_RANGE,
    STATUS_ERR_STORE_FULL,
    STATUS_ERR_UNKNOWN_ID,
    STATUS_ERR_BAD_LENGTH,
    STATUS_ERR_BAD_CHECKSUM
} Status_t;

// The choices offered by the main menu.
typedef enum
{
    MENU_EXIT = 0,
    MENU_LOAD_FILE,
    MENU_DECODE,
    MENU_STATISTICS,
    MENU_FRAMES_BY_ID,
    MENU_ERROR_FRAMES,
    MENU_SINGLE_FRAME,
    MENU_INVALID
} MenuChoice_t;


// Structures                                                        


// One CAN frame as read from the log. data[7] is the checksum. 
typedef struct
{
    uint32_t timeMs;
    uint16_t id;
    uint8_t  dlc;
    uint8_t  data[DATA_BYTES];
} CanFrame_t;

// Where one signal sits in the data bytes and how to convert its value. 
typedef struct
{
    const char *name;
    const char *unit;
    uint8_t startBit;
    uint8_t bitLength;
    float factor;
    float offset;
} SignalDefinition_t;

// One known message and its signals.
typedef struct
{
    uint16_t id;
    const char *name;
    uint8_t signalCount;
    SignalDefinition_t signals[MAX_SIGNALS_PER_MESSAGE];
} MessageDefinition_t;

// The physical values decoded from one frame. 
typedef struct
{
    uint8_t count;
    float values[MAX_SIGNALS_PER_MESSAGE];
} DecodedValues_t;

// One processed frame kept in memory.
typedef struct
{
    CanFrame_t frame;
    uint32_t lineNumber;
    Status_t status;
    DecodedValues_t decoded;
} StoredFrame_t;

// The fixed-size store of processed frames. 
typedef struct
{
    StoredFrame_t items[MAX_FRAMES];
    uint16_t count;
} FrameStore_t;

// Statistics for one identifier.
typedef struct
{
    uint16_t id;
    uint32_t received;
    uint32_t valid;
    uint32_t errors;
} IdStatistics_t;

// Counters for the summary report.
typedef struct
{
    uint32_t linesRead;
    uint32_t linesRejected;
    uint32_t framesDecoded;
    uint32_t unknownIdErrors;
    uint32_t lengthErrors;
    uint32_t checksumErrors;
} Summary_t;

#ifdef __cplusplus
}
#endif

#endif // TYPES_H
