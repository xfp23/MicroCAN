
[EN](./readme.md)

# MicroCAN

轻量级 CAN 报文打包 / 解包库，适用于嵌入式系统。

---

## 特性

- 支持 Intel（小端）与 Motorola（大端）两种字节序
- 统一的信号读写 API，索引访问，无需直接操作结构体成员
- 支持有符号 / 无符号信号，以及 factor / offset 物理值换算
- 纯 C 实现，兼容 C99，无动态内存分配

---

## 文件结构

```
MicroCAN/
├── MicroCAN.h          # 公共 API 声明
├── MicroCAN_types.h    # 数据结构与枚举定义
└── MicroCAN.c          # 实现文件
```

---

## 快速上手

### 1. 定义信号表

```c
/* 用枚举做信号索引，可读性等同于成员名 */
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

### 2. 初始化报文

```c
MicroCAN_Message_t engine_msg = {
    .msg_id  = 0x100,
    .dlc     = 4,
    .sig_num = SIG_COUNT,
};

MicroCAN_Init(&engine_msg, engine_signals);
```

### 3. 发送：赋值 → 打包

```c
double speed = 3000.0;
double temp  = 90.0;

MicroCAN_SetSignalValue(&engine_msg, &speed, SIG_ENGINE_SPEED);
MicroCAN_SetSignalValue(&engine_msg, &temp,  SIG_ENGINE_TEMP);

MicroCAN_Pack(&engine_msg);

/* engine_msg.msg_data 即可直接送入硬件 CAN 驱动 */
HAL_CAN_Transmit(&engine_msg.msg_data, engine_msg.dlc);
```

### 4. 接收：填入原始数据 → 解包 → 取值

```c
/* 直接将硬件帧数据传给 UnPack，无需手动 memcpy */
MicroCAN_UnPack(&engine_msg, rx_frame.data, rx_frame.dlc);

double speed_out, temp_out;
MicroCAN_GetSignalValue(&engine_msg, &speed_out, SIG_ENGINE_SPEED);
MicroCAN_GetSignalValue(&engine_msg, &temp_out,  SIG_ENGINE_TEMP);
```

---

## API 参考

### MicroCAN_Init

```c
MicroCAN_Status_t MicroCAN_Init(MicroCAN_Message_t *msg, const MicroCAN_Signal_t *sig);
```

将信号表关联到报文对象。必须在 Pack / Unpack 之前调用。

| 参数 | 说明 |
|------|------|
| `msg` | 报文对象指针 |
| `sig` | 信号表指针（数组首地址） |

---

### MicroCAN_SetSignalValue

```c
MicroCAN_Status_t MicroCAN_SetSignalValue(MicroCAN_Message_t *msg, void *value, size_t index);
```

按索引设置信号的物理值。库内部根据信号的 `type` 字段做类型派发，将 `value` 所指变量转换为 `double` 存入信号。**传入指针的实际类型必须与信号定义的 `type` 一致**，否则行为未定义。

| 参数 | 说明 |
|------|------|
| `msg`   | 报文对象指针 |
| `value` | 待写入的物理值指针，类型须与信号 `type` 匹配 |
| `index` | 信号索引（对应枚举值） |

```c
// 示例：信号 type 为 MICROCAN_SIG_FLOAT
float spd = 3000.0f;
MicroCAN_SetSignalValue(&engine_msg, &spd, SIG_ENGINE_SPEED);

// 示例：信号 type 为 MICROCAN_SIG_DOUBLE（或 MICROCAN_SIG_DEFAULT）
double temp = 90.0;
MicroCAN_SetSignalValue(&engine_msg, &temp, SIG_ENGINE_TEMP);
```

---

### MicroCAN_GetSignalValue

```c
MicroCAN_Status_t MicroCAN_GetSignalValue(const MicroCAN_Message_t *msg, void *value, size_t index);
```

按索引读取信号的物理值，根据信号 `type` 将内部 `double` 强制转换后写入 `value` 所指变量。**输出缓冲区类型必须与信号定义的 `type` 一致**。

| 参数 | 说明 |
|------|------|
| `msg`   | 报文对象指针 |
| `value` | 输出缓冲区指针，类型须与信号 `type` 匹配 |
| `index` | 信号索引 |

```c
// 示例：信号 type 为 MICROCAN_SIG_U16
uint16_t rpm;
MicroCAN_GetSignalValue(&engine_msg, &rpm, SIG_ENGINE_SPEED);

// 示例：信号 type 为 MICROCAN_SIG_DOUBLE（或 MICROCAN_SIG_DEFAULT）
double temp;
MicroCAN_GetSignalValue(&engine_msg, &temp, SIG_ENGINE_TEMP);
```

---

### MicroCAN_Pack

```c
MicroCAN_Status_t MicroCAN_Pack(MicroCAN_Message_t *msg);
```

将所有信号的物理值按照 factor / offset 换算为原始值，并编码写入 `msg->msg_data`。

---

### MicroCAN_UnPack

```c
MicroCAN_Status_t MicroCAN_UnPack(MicroCAN_Message_t *msg, const uint8_t *data, size_t len);
```

将外部原始帧数据解码为所有信号的物理值，写回各信号的 `value` 字段。`data` 直接传入硬件驱动的接收缓冲区，无需调用方手动 `memcpy`。

| 参数 | 说明 |
|------|------|
| `msg`  | 报文对象指针 |
| `data` | CAN 接收帧数据指针（硬件驱动提供） |
| `len`  | 接收数据长度（字节数） |

---

### 报文属性访问宏

以下宏以值语义访问报文的只读属性，适合在驱动适配层或发送回调中使用。

#### MicroCAN_GetMessageId

```c
#define MicroCAN_GetMessageId(msg)   ((msg).msg_id)
```

返回报文的 CAN 标识符（`uint32_t`）。

```c
uint32_t id = MicroCAN_GetMessageId(engine_msg);  // 0x100
```

---

#### MicroCAN_GetFrameData

```c
#define MicroCAN_GetFrameData(msg)   ((msg).msg_data)
```

返回报文的原始数据缓冲区首地址（`uint8_t *`），Pack 之后直接传给硬件驱动。

```c
HAL_CAN_Transmit(MicroCAN_GetFrameData(engine_msg), MicroCAN_GetMessageDLC(engine_msg));
```

---

#### MicroCAN_GetMessageDLC

```c
#define MicroCAN_GetMessageDLC(msg)  ((msg).dlc)
```

返回报文的 DLC（数据长度码，`uint8_t`），标准 CAN 帧取值范围 0 ~ 8。

---

### 信号类型（MicroCAN_SigType_t）

信号的 `type` 字段决定了 `MicroCAN_SetSignalValue` / `MicroCAN_GetSignalValue` 在做类型转换时的目标类型。定义信号表时必须与业务层变量的实际类型对应，否则 `void *` 解引用行为未定义。

| 枚举值 | 对应 C 类型 | 典型用途 |
|--------|------------|---------|
| `MICROCAN_SIG_U8`     | `uint8_t`  | 状态位、档位、开关量 |
| `MICROCAN_SIG_U16`    | `uint16_t` | 转速、电压（整数精度） |
| `MICROCAN_SIG_U32`    | `uint32_t` | 里程、时间戳 |
| `MICROCAN_SIG_INT`    | `int`      | 有符号整数信号 |
| `MICROCAN_SIG_FLOAT`  | `float`    | 温度、电流（单精度） |
| `MICROCAN_SIG_DOUBLE` | `double`   | 高精度物理量 |
| `MICROCAN_SIG_DEFAULT`| `double`   | 未指定类型时的默认处理，等价于 `DOUBLE` |

> **注意**：所有类型在库内部统一以 `double` 存储（`signal.value` 字段），`type` 仅影响 Set/Get 时的入口强制转换，不影响 Pack/Unpack 的位操作。

---

### 返回值

| 枚举值 | 含义 |
|--------|------|
| `MICROCAN_OK` | 操作成功 |
| `MICROCAN_ERR` | 通用错误 |
| `MICROCAN_PARAMERR` | 参数无效（空指针等） |
| `MICROCAN_OVERDLC_ERR` | 超出 DLC 范围 |
| `MICROCAN_BYTEORDER_ERR` | 未知字节序 |
| `MICROCAN_BYTELEN_ERR` | 信号位长度越界 |
| `MICROCAN_INDEX_ERR` | 访问索引溢出 |
| `MICROCAN_REJECT_ERR` | 报文状态出错，库拒绝操作 |

---

## 物理值换算公式

```
打包（物理值 → 原始值）：raw = (value - offset) / factor
解包（原始值 → 物理值）：value = raw * factor + offset
```

---

## 注意事项

- `sig_num` 必须与实际信号数组长度一致，否则 Pack / Unpack 会越界。
- `factor` 不可为 0，否则 Pack 将返回 `MICROCAN_ERR`。
- 信号 `value` 在 Pack 时会自动限幅到 `[Min, Max]`。
- 库内部不进行动态内存分配，信号数组生命周期由调用方管理。

---

## License

Copyright (c) 2026 xfp23. All rights reserved.
