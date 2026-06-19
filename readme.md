
[中文](./readme.zh.md)

# MicroCAN

A lightweight CAN message packing and unpacking library for embedded systems.

---

## Features

- Intel (little-endian) and Motorola (big-endian) byte order support
- Unified signal read/write API via index — no direct struct member access required
- Signed/unsigned signal support with physical value conversion (factor / offset)
- Pure C99 implementation, no dynamic memory allocation

---

## File Structure

```
MicroCAN/
├── MicroCAN.h          # Public API declarations
├── MicroCAN_types.h    # Data structures and enumerations
└── MicroCAN.c          # Implementation
```

---

## Quick Start

### 1. Define a Signal Table

```c
/* Use an enum for signal indices — readable like member names */
typedef enum {
    SIG_ENGINE_SPEED = 0,
    SIG_ENGINE_TEMP,
    SIG_COUNT
} EngineSpeed_SigIdx_t;

static MicroCAN_Signal_t engine_signals[SIG_COUNT] = {
    [SIG_ENGINE_SPEED] = {
        .start_bits = 0,
        .length     = 16,
        .byte_order = MICROCAN_BYTEORDER_INTEL,
        .type = MICROCAN_SIG_FLOAT,
        .is_signed  = false,
        .factor     = 0.1f,
        .offset     = 0.0f,
        .Min        = 0.0f,
        .Max        = 6500.0f,
    },
    [SIG_ENGINE_TEMP] = {
        .start_bits = 16,
        .length     = 8,
        .byte_order = MICROCAN_BYTEORDER_INTEL,
        .type = MICROCAN_SIG_FLOAT,
        .is_signed  = true,
        .factor     = 1.0f,
        .offset     = -40.0f,
        .Min        = -40.0f,
        .Max        = 215.0f,
    },
};
```

### 2. Initialize the Message

```c
MicroCAN_Message_t engine_msg = {
    .msg_id  = 0x100,
    .dlc     = 4,
    .sig_num = SIG_COUNT,
};

MicroCAN_Init(&engine_msg, engine_signals);
```

### 3. Transmit: Set Values → Pack

```c
double speed = 3000.0;
double temp  = 90.0;

MicroCAN_SetSignalValue(&engine_msg, &speed, SIG_ENGINE_SPEED);
MicroCAN_SetSignalValue(&engine_msg, &temp,  SIG_ENGINE_TEMP);

MicroCAN_Pack(&engine_msg);

/* engine_msg.msg_data is now ready to pass to the hardware CAN driver */
HAL_CAN_Transmit(&engine_msg.msg_data, engine_msg.dlc);
```

### 4. Receive: Load Raw Data → Unpack → Read Values

```c
/* Pass the hardware frame buffer directly — no manual memcpy needed */
MicroCAN_UnPack(&engine_msg, rx_frame.data, rx_frame.dlc);

double speed_out, temp_out;
MicroCAN_GetSignalValue(&engine_msg, &speed_out, SIG_ENGINE_SPEED);
MicroCAN_GetSignalValue(&engine_msg, &temp_out,  SIG_ENGINE_TEMP);
```

---

## API Reference

### MicroCAN_Init

```c
MicroCAN_Status_t MicroCAN_Init(MicroCAN_Message_t *msg, const MicroCAN_Signal_t *sig);
```

Associates a signal table with a message object. Must be called before Pack or Unpack.

| Parameter | Description |
|-----------|-------------|
| `msg` | Pointer to the message object |
| `sig` | Pointer to the signal table (array base address) |

---

### MicroCAN_SetSignalValue

```c
MicroCAN_Status_t MicroCAN_SetSignalValue(MicroCAN_Message_t *msg, void *value, size_t index);
```

Sets the physical value of a signal by index. The library dispatches on the signal's `type` field to cast the incoming pointer and store the result as `double` internally. **The actual type of the variable pointed to by `value` must match the signal's `type` — mismatches cause undefined behavior.**

| Parameter | Description |
|-----------|-------------|
| `msg`   | Pointer to the message object |
| `value` | Pointer to the physical value; type must match the signal's `type` field |
| `index` | Signal index (corresponds to the enum value) |

```c
// Signal type is MICROCAN_SIG_FLOAT
float spd = 3000.0f;
MicroCAN_SetSignalValue(&engine_msg, &spd, SIG_ENGINE_SPEED);

// Signal type is MICROCAN_SIG_DOUBLE (or MICROCAN_SIG_DEFAULT)
double temp = 90.0;
MicroCAN_SetSignalValue(&engine_msg, &temp, SIG_ENGINE_TEMP);
```

---

### MicroCAN_GetSignalValue

```c
MicroCAN_Status_t MicroCAN_GetSignalValue(const MicroCAN_Message_t *msg, void *value, size_t index);
```

Reads the physical value of a signal by index. The internal `double` is cast to the signal's `type` and written into the buffer pointed to by `value`. **The output buffer type must match the signal's `type` field.**

| Parameter | Description |
|-----------|-------------|
| `msg`   | Pointer to the message object |
| `value` | Output buffer pointer; type must match the signal's `type` field |
| `index` | Signal index |

```c
// Signal type is MICROCAN_SIG_U16
uint16_t rpm;
MicroCAN_GetSignalValue(&engine_msg, &rpm, SIG_ENGINE_SPEED);

// Signal type is MICROCAN_SIG_DOUBLE (or MICROCAN_SIG_DEFAULT)
double temp;
MicroCAN_GetSignalValue(&engine_msg, &temp, SIG_ENGINE_TEMP);
```

---

### MicroCAN_Pack

```c
MicroCAN_Status_t MicroCAN_Pack(MicroCAN_Message_t *msg);
```

Converts all signal physical values to raw values using the factor/offset formula and encodes them into `msg->msg_data`.

---

### MicroCAN_UnPack

```c
MicroCAN_Status_t MicroCAN_UnPack(MicroCAN_Message_t *msg, const uint8_t *data, size_t len);
```

Decodes an external raw frame buffer into physical signal values and writes them back into each signal's `value` field. The `data` pointer can be passed directly from the hardware driver's receive buffer — no `memcpy` required.

| Parameter | Description |
|-----------|-------------|
| `msg`  | Pointer to the message object |
| `data` | Pointer to the received CAN frame data (from hardware driver) |
| `len`  | Length of the received data in bytes |

---

### Message Attribute Macros

The following macros provide value-semantics access to read-only message properties. They are intended for use in driver adaptation layers or transmit callbacks.

#### MicroCAN_GetMessageId

```c
#define MicroCAN_GetMessageId(msg)   ((msg).msg_id)
```

Returns the CAN identifier of the message (`uint32_t`).

```c
uint32_t id = MicroCAN_GetMessageId(engine_msg);  // 0x100
```

---

#### MicroCAN_GetFrameData

```c
#define MicroCAN_GetFrameData(msg)   ((msg).msg_data)
```

Returns a pointer to the raw data buffer (`uint8_t *`). After calling `MicroCAN_Pack`, pass this directly to the hardware CAN driver.

```c
HAL_CAN_Transmit(MicroCAN_GetFrameData(engine_msg), MicroCAN_GetMessageDLC(engine_msg));
```

---

#### MicroCAN_GetMessageDLC

```c
#define MicroCAN_GetMessageDLC(msg)  ((msg).dlc)
```

Returns the DLC (Data Length Code) of the message (`uint8_t`). Valid range for standard CAN frames: 0 – 8.

---

### Signal Type (MicroCAN_SigType_t)

The `type` field on each signal controls the cast performed by `MicroCAN_SetSignalValue` and `MicroCAN_GetSignalValue`. It must match the actual C type of the caller's variable; a mismatch on a `void *` dereference is undefined behavior.

| Value | C Type | Typical Use |
|-------|--------|-------------|
| `MICROCAN_SIG_U8`     | `uint8_t`  | Status bits, gear position, switches |
| `MICROCAN_SIG_U16`    | `uint16_t` | RPM, voltage (integer precision) |
| `MICROCAN_SIG_U32`    | `uint32_t` | Odometer, timestamps |
| `MICROCAN_SIG_INT`    | `int`      | Signed integer signals |
| `MICROCAN_SIG_FLOAT`  | `float`    | Temperature, current (single precision) |
| `MICROCAN_SIG_DOUBLE` | `double`   | High-precision physical values |
| `MICROCAN_SIG_DEFAULT`| `double`   | Unspecified type; treated identically to `DOUBLE` |

> **Note:** All signal values are stored internally as `double` (`signal.value`). The `type` field only affects the entry-point cast in Set/Get — it has no effect on the bit-level Pack/Unpack operations.

---

### Return Values

| Value | Meaning |
|-------|---------|
| `MICROCAN_OK` | Operation succeeded |
| `MICROCAN_ERR` | General error |
| `MICROCAN_PARAMERR` | Invalid parameter (null pointer, etc.) |
| `MICROCAN_OVERDLC_ERR` | DLC exceeded |
| `MICROCAN_BYTEORDER_ERR` | Unknown byte order |
| `MICROCAN_BYTELEN_ERR` | Signal bit position out of range |
| `MICROCAN_INDEX_ERR` | Access index overflow |
| `MICROCAN_REJECT_ERR` | Message status error, operation refused |

---

## Physical Value Conversion

```
Pack   (physical → raw):   raw   = (value - offset) / factor
Unpack (raw → physical):   value = raw * factor + offset
```

---

## Notes

- `sig_num` must match the actual length of the signal array; a mismatch will cause Pack/Unpack to access out-of-bounds memory.
- `factor` must not be zero; Pack will return `MICROCAN_ERR` if it is.
- Signal values are automatically clamped to `[Min, Max]` during Pack.
- The library performs no dynamic memory allocation; the signal array lifetime is managed by the caller.

---

## License

Copyright (c) 2026 xfp23. All rights reserved.
