# Detailed Design: CAN Log Decoder

## Document Overview

This document turns the requirements in `project_proposal.md` into a detailed
design. The code, tests and Makefile shall be written against it.

| Section | Content |
|---|---|
| 1 | Data design: constants, enumerations, structures, signal table, data dictionary |
| 2 | Module design |
| 3 | Function design |
| 4 | Input-validation plan |
| 5 | File-handling design |
| 6 | Error-handling strategy |
| 7 | Coding standards plan |
| 8 | Application flow |
| 9 | Unit-test plan |
| 10 | Makefile design |
| 11 | Requirement traceability |

The data design comes first because every other section refers to its types.

**Naming conventions used throughout.** Functions use camelCase (`validateFrame`),
types end in `_t` (`CanFrame_t`), constants and macros use UPPER_CASE
(`MAX_FRAMES`) and file names use snake_case (`file_handler.c`).

---

## 1. Data Design

### 1.1 Constants

| Constant | Value | Purpose |
|---|---|---|
| `MAX_FRAMES` | 1000 | Maximum number of frames kept in memory |
| `DATA_BYTES` | 8 | Number of data bytes in every frame |
| `FIELDS_PER_LINE` | 11 | Fields in a CSV data line: time, identifier, length and 8 data bytes |
| `CHECKSUM_INDEX` | 7 | Index of the checksum byte in the data bytes |
| `MAX_CAN_ID` | 0x7FF | Largest 11-bit identifier |
| `MESSAGE_COUNT` | 3 | Number of known messages |
| `MAX_SIGNALS_PER_MESSAGE` | 3 | Largest number of signals in one message |
| `MAX_LINE_LENGTH` | 256 | Size of the buffer that holds one line of text |
| `MAX_FILE_NAME_LENGTH` | 128 | Size of the buffer that holds a file name |
| `ID_ENGINE_DATA` | 0x100 | Identifier of the engine data message |
| `ID_VEHICLE_SPEED` | 0x200 | Identifier of the vehicle speed message |
| `ID_BATTERY_STATUS` | 0x300 | Identifier of the battery status message |

### 1.2 Enumeration Definitions

#### Enumeration: Status_t

**Purpose:** one result type for every function that can fail. It is also stored
with each frame, where it records the first check the frame failed.

```c
typedef enum
{
    STATUS_SUCCESS = 0,
    STATUS_END_OF_FILE,
    STATUS_ERR_NULL_POINTER,
    STATUS_ERR_INVALID_ARGUMENT,
    STATUS_ERR_FILE_OPEN,
    STATUS_ERR_FILE_EMPTY,
    STATUS_ERR_LINE_TOO_LONG,
    STATUS_ERR_FIELD_COUNT,
    STATUS_ERR_INVALID_HEX,
    STATUS_ERR_OUT_OF_RANGE,
    STATUS_ERR_STORE_FULL,
    STATUS_ERR_UNKNOWN_ID,
    STATUS_ERR_BAD_LENGTH,
    STATUS_ERR_BAD_CHECKSUM
} Status_t;
```

| Constant | Meaning |
|---|---|
| `STATUS_SUCCESS` | The operation succeeded, or the frame passed every check |
| `STATUS_END_OF_FILE` | No more lines to read. This is not an error |
| `STATUS_ERR_NULL_POINTER` | A required pointer argument was NULL |
| `STATUS_ERR_INVALID_ARGUMENT` | An argument had an unacceptable value, for example an empty file name |
| `STATUS_ERR_FILE_OPEN` | The file does not exist or cannot be opened |
| `STATUS_ERR_FILE_EMPTY` | The file has no header line |
| `STATUS_ERR_LINE_TOO_LONG` | A line is longer than `MAX_LINE_LENGTH` |
| `STATUS_ERR_FIELD_COUNT` | A line does not have exactly `FIELDS_PER_LINE` fields |
| `STATUS_ERR_INVALID_HEX` | A field is empty or is not valid hexadecimal or decimal text |
| `STATUS_ERR_OUT_OF_RANGE` | A number is outside its allowed range |
| `STATUS_ERR_STORE_FULL` | The frame store already holds `MAX_FRAMES` frames |
| `STATUS_ERR_UNKNOWN_ID` | The identifier is not in the table of known messages |
| `STATUS_ERR_BAD_LENGTH` | The data length is not 8 |
| `STATUS_ERR_BAD_CHECKSUM` | Byte 7 does not equal the XOR of bytes 0 to 6 |

#### Enumeration: MenuChoice_t

**Purpose:** gives the menu options readable names.

```c
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
```

| Constant | Value | Meaning |
|---|---|---|
| `MENU_EXIT` | 0 | End the program |
| `MENU_LOAD_FILE` | 1 | Ask for a file name and check that the file can be read |
| `MENU_DECODE` | 2 | Process every line of the loaded file |
| `MENU_STATISTICS` | 3 | Show statistics for each identifier |
| `MENU_FRAMES_BY_ID` | 4 | Show the stored frames of one identifier |
| `MENU_ERROR_FRAMES` | 5 | Show the stored frames that failed a check |
| `MENU_SINGLE_FRAME` | 6 | Validate and decode one frame entered by the user |
| `MENU_INVALID` | 7 | Returned when the input is not a valid choice |

### 1.3 Structure Definitions

#### Structure: CanFrame_t

**Purpose:** one CAN frame as read from the log.

```c
typedef struct
{
    uint32_t timeMs;
    uint16_t id;
    uint8_t  dlc;
    uint8_t  data[DATA_BYTES];
} CanFrame_t;
```

| Member | C Type | Valid Range or Size | Description |
|---|---|---|---|
| `timeMs` | `uint32_t` | 0 to 4294967295 | Time stamp in milliseconds |
| `id` | `uint16_t` | 0 to 0x7FF | 11-bit identifier |
| `dlc` | `uint8_t` | 0 to 8 as parsed, 8 for a valid frame | Data length code |
| `data` | `uint8_t[8]` | 0x00 to 0xFF each | Data bytes. `data[7]` is the checksum |

#### Structure: SignalDefinition_t

**Purpose:** describes where one signal sits inside the data bytes and how to
convert its raw value into a physical value.

```c
typedef struct
{
    const char *name;
    const char *unit;
    uint8_t     startBit;
    uint8_t     bitLength;
    float       factor;
    float       offset;
} SignalDefinition_t;
```

| Member | C Type | Valid Range or Size | Description |
|---|---|---|---|
| `name` | `const char *` | Non-NULL text | Signal name |
| `unit` | `const char *` | Non-NULL text | Unit, or an empty string |
| `startBit` | `uint8_t` | 0 to 63 | Position of the signal's lowest bit (bit 0 is the lowest bit of `data[0]`) |
| `bitLength` | `uint8_t` | 1 to 16 | Number of bits in the signal |
| `factor` | `float` | Non-zero | Multiplier applied to the raw value |
| `offset` | `float` | Any | Value added after multiplying |

#### Structure: MessageDefinition_t

**Purpose:** describes one known message and its signals.

```c
typedef struct
{
    uint16_t           id;
    const char        *name;
    uint8_t            signalCount;
    SignalDefinition_t signals[MAX_SIGNALS_PER_MESSAGE];
} MessageDefinition_t;
```

| Member | C Type | Valid Range or Size | Description |
|---|---|---|---|
| `id` | `uint16_t` | 0 to 0x7FF | Identifier of the message |
| `name` | `const char *` | Non-NULL text | Message name |
| `signalCount` | `uint8_t` | 1 to 3 | Number of signals used in the array |
| `signals` | `SignalDefinition_t[3]` | See `signalCount` | The signals of the message |

#### Structure: DecodedValues_t

**Purpose:** the physical values obtained from one frame.

```c
typedef struct
{
    uint8_t count;
    float   values[MAX_SIGNALS_PER_MESSAGE];
} DecodedValues_t;
```

| Member | C Type | Valid Range or Size | Description |
|---|---|---|---|
| `count` | `uint8_t` | 0 to 3 | Number of valid values. It is 0 for a frame that was not decoded |
| `values` | `float[3]` | Any | Decoded values, in the order of the signal table |

#### Structure: StoredFrame_t

**Purpose:** one processed frame kept in memory.

```c
typedef struct
{
    CanFrame_t      frame;
    uint32_t        lineNumber;
    Status_t        status;
    DecodedValues_t decoded;
} StoredFrame_t;
```

| Member | C Type | Valid Range or Size | Description |
|---|---|---|---|
| `frame` | `CanFrame_t` | See `CanFrame_t` | The frame as read |
| `lineNumber` | `uint32_t` | 2 or more | Line of the log file (the header is line 1) |
| `status` | `Status_t` | `STATUS_SUCCESS` or the first failed check | Result of validation |
| `decoded` | `DecodedValues_t` | See `DecodedValues_t` | Decoded values, present only if the frame is valid |

#### Structure: FrameStore_t

**Purpose:** the fixed-size store of up to 1000 processed frames.

```c
typedef struct
{
    StoredFrame_t items[MAX_FRAMES];
    uint16_t      count;
} FrameStore_t;
```

| Member | C Type | Valid Range or Size | Description |
|---|---|---|---|
| `items` | `StoredFrame_t[1000]` | Only the first `count` are used | The stored frames in the order read |
| `count` | `uint16_t` | 0 to 1000 | Number of frames stored |

The structure is about 40 KB, which is acceptable as a local variable in
`main`. It is passed to functions by pointer.

#### Structure: IdStatistics_t

**Purpose:** the statistics for one identifier.

```c
typedef struct
{
    uint16_t id;
    uint32_t received;
    uint32_t valid;
    uint32_t errors;
} IdStatistics_t;
```

| Member | C Type | Valid Range or Size | Description |
|---|---|---|---|
| `id` | `uint16_t` | 0 to 0x7FF | The identifier |
| `received` | `uint32_t` | 0 to 1000 | Stored frames with this identifier |
| `valid` | `uint32_t` | 0 to `received` | Frames that passed every check |
| `errors` | `uint32_t` | 0 to `received` | Frames that failed a check |

#### Structure: Summary_t

**Purpose:** the counters for the summary report.

```c
typedef struct
{
    uint32_t linesRead;
    uint32_t linesRejected;
    uint32_t framesDecoded;
    uint32_t unknownIdErrors;
    uint32_t lengthErrors;
    uint32_t checksumErrors;
} Summary_t;
```

| Member | C Type | Valid Range or Size | Description |
|---|---|---|---|
| `linesRead` | `uint32_t` | 0 or more | Data lines read (the header is not counted) |
| `linesRejected` | `uint32_t` | 0 to `linesRead` | Lines that could not be parsed |
| `framesDecoded` | `uint32_t` | 0 to `linesRead` | Frames that passed every check and were decoded |
| `unknownIdErrors` | `uint32_t` | 0 or more | Frames with an unknown identifier |
| `lengthErrors` | `uint32_t` | 0 or more | Frames whose data length is not 8 |
| `checksumErrors` | `uint32_t` | 0 or more | Frames with a wrong checksum |

### 1.4 Signal Table

The decoder holds this table as a constant array of `MessageDefinition_t`.
Identifiers, positions and conversions are changed here only (NFR-07).

**Bit numbering.** Signals are unsigned and little-endian. Bit 0 is the lowest
bit of `data[0]`, bit 8 is the lowest bit of `data[1]`, and so on. The raw value
of a signal is taken from `bitLength` bits starting at `startBit`, and

```
physical value = raw value * factor + offset
```

| Message | ID | Signal | Unit | Start bit | Length | Factor | Offset | Meaning |
|---|---|---|---|---|---|---|---|---|
| EngineData | 0x100 | engine speed | rpm | 0 | 16 | 0.25 | 0 | Bytes 0 and 1 |
| EngineData | 0x100 | coolant temperature | deg C | 16 | 8 | 1 | -40 | Byte 2 |
| EngineData | 0x100 | throttle position | % | 24 | 8 | 0.4 | 0 | Byte 3 |
| VehicleSpeed | 0x200 | vehicle speed | km/h | 0 | 16 | 0.01 | 0 | Bytes 0 and 1 |
| VehicleSpeed | 0x200 | brake pressed | none | 16 | 1 | 1 | 0 | Bit 0 of byte 2, 1 means ON |
| VehicleSpeed | 0x200 | gear | none | 17 | 3 | 1 | 0 | Bits 1 to 3 of byte 2 |
| BatteryStatus | 0x300 | battery voltage | V | 0 | 8 | 0.1 | 0 | Byte 0 |

**Worked example.** `1A 2C 78 80 00 00 00 CE` on identifier 0x100:

| Signal | Raw value | Physical value |
|---|---|---|
| engine speed | `0x2C1A` = 11290 | 11290 x 0.25 = 2822.5 rpm |
| coolant temperature | `0x78` = 120 | 120 - 40 = 80 deg C |
| throttle position | `0x80` = 128 | 128 x 0.4 = 51.2 % |
| checksum | 0x1A ^ 0x2C ^ 0x78 ^ 0x80 ^ 0 ^ 0 ^ 0 = 0xCE | equals byte 7, so the frame is valid |

### 1.5 Data Dictionary

| Data Item | C Type | Valid Range or Size | Validation Rule | Description |
|---|---|---|---|---|
| time stamp | `uint32_t` | 0 to 4294967295 | Decimal digits only, no sign | Time of the frame in milliseconds |
| identifier | `uint16_t` | 0 to 0x7FF | Hexadecimal text with an optional `0x` prefix. Above 0x7FF is out of range. Must be one of 0x100, 0x200 or 0x300 to be decoded | Message identifier |
| data length | `uint8_t` | 0 to 8 | Decimal digits only. Above 8 is out of range. Must equal 8 for a valid frame | Data length code |
| data byte | `uint8_t` | 0x00 to 0xFF | One or two hexadecimal digits, no prefix | One of the eight data bytes |
| checksum | `uint8_t` | 0x00 to 0xFF | Must equal the XOR of data bytes 0 to 6 | Data byte 7 |
| line number | `uint32_t` | 2 or more | Counted by the file handler | Position of the line in the file |
| frame status | `Status_t` | Any `Status_t` value | Set by the validation module | Result of the checks on a frame |
| raw value | `uint32_t` | 0 to 65535 | Extracted from `bitLength` bits | Unsigned value of a signal before conversion |
| physical value | `float` | Any finite number | Calculated from the raw value | Signal value in its unit |
| file name | `char[128]` | 1 to 127 characters | Not empty, no newline, and the file can be opened | Name of the log file |
| menu input | `char[16]` | 1 to 15 characters | A whole number from 0 to 6 with nothing else | Text typed at the menu |
| selected identifier | `uint16_t` | 0 to 0x7FF | Hexadecimal text with an optional `0x` prefix | Identifier chosen for filtering |
| line buffer | `char[256]` | Up to 255 characters | Longer lines are rejected | One line of the log file |
| stored frame count | `uint16_t` | 0 to 1000 | A frame is added only while the count is below 1000 | Number of frames in the store |

### 1.6 Design Decisions Recorded Here

1. **A line that cannot be parsed is rejected and not stored.** It is counted in
   `linesRejected` and reported with its line number.
2. **A frame that parses but fails a check is stored.** Its `status` records
   which check failed, so that the error frames can be listed later.
3. **The checks run in a fixed order:** identifier, then data length, then
   checksum. The status of a frame is the first check that failed. The checksum
   is not examined if the length is wrong, because the byte positions are then
   not reliable.
4. **Only frames with the status `STATUS_SUCCESS` are decoded.** For all other
   frames `decoded.count` is 0.
5. **Time stamps are assumed to increase** (section 3.3 of the proposal). The
   program does not check this.
6. **Signals are described by data, not by code.** The decoder uses one generic
   bit-extraction routine and the signal table, so adding a signal means adding
   a table row.
