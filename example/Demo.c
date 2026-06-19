/**
 * @file demo.c
 * @brief MicroCAN 使用示例
 *
 * 本文件演示两条典型 CAN 报文的完整使用流程：
 *
 *   报文 1 — 0x100  发动机状态（Engine Status）
 *     信号: engine_speed  [bit 0~15,  Intel, U16, factor=0.1,  offset=0,   0~6500 rpm]
 *     信号: engine_temp   [bit 16~23, Intel, INT, factor=1.0,  offset=-40, -40~215 °C]
 *     信号: engine_mode   [bit 24~27, Intel, U8,  factor=1.0,  offset=0,   0~15      ]
 *
 *   报文 2 — 0x200  车速与加速度（Vehicle Dynamics）
 *     信号: vehicle_speed [bit 0~15,  Motorola, FLOAT,  factor=0.01, offset=0, 0~300 km/h]
 *     信号: accel_x       [bit 16~31, Motorola, DOUBLE, factor=0.001,offset=0, -20~20 m/s²]
 *
 * 流程：
 *   1. 定义信号表与报文对象
 *   2. 初始化
 *   3. 发送侧：Set → Pack → 送入（模拟）驱动
 *   4. 接收侧：（模拟）驱动给出原始数据 → UnPack → Get
 *   5. 错误处理示例
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "MicroCAN.h"

/* =========================================================
 * 工具宏：检查返回值，失败时打印错误并返回
 * ========================================================= */
#define CHECK_OK(expr)                                              \
    do {                                                            \
        MicroCAN_Status_t _s = (expr);                             \
        if (_s != MICROCAN_OK) {                                    \
            printf("[ERROR] %s => status %d\n", #expr, (int)_s);   \
            return _s;                                              \
        }                                                           \
    } while (0)

/* =========================================================
 * 报文 1 — 0x100  发动机状态
 * ========================================================= */

/* 信号索引枚举，可读性等同于成员名 */
typedef enum {
    SIG_ENGINE_SPEED = 0,
    SIG_ENGINE_TEMP,
    SIG_ENGINE_MODE,
    SIG_ENGINE_COUNT,           /* 信号总数，用于 sig_num */
} EngineStatus_SigIdx_t;

/* 信号描述表：所有配置字段为 const，运行时不可修改 */
static MicroCAN_Signal_t engine_signals[SIG_ENGINE_COUNT] = {
    [SIG_ENGINE_SPEED] = {
        .start_bits = 0,
        .length     = 16,
        .byte_order = MICROCAN_BYTEORDER_INTEL,
        .is_signed  = false,
        .factor     = 0.1f,
        .offset     = 0.0f,
        .Min        = 0.0f,
        .Max        = 6500.0f,
        .type       = MICROCAN_SIG_U16,
    },
    [SIG_ENGINE_TEMP] = {
        .start_bits = 16,
        .length     = 8,
        .byte_order = MICROCAN_BYTEORDER_INTEL,
        .is_signed  = true,
        .factor     = 1.0f,
        .offset     = -40.0f,
        .Min        = -40.0f,
        .Max        = 215.0f,
        .type       = MICROCAN_SIG_INT,
    },
    [SIG_ENGINE_MODE] = {
        .start_bits = 24,
        .length     = 4,
        .byte_order = MICROCAN_BYTEORDER_INTEL,
        .is_signed  = false,
        .factor     = 1.0f,
        .offset     = 0.0f,
        .Min        = 0.0f,
        .Max        = 15.0f,
        .type       = MICROCAN_SIG_U8,
    },
};

static MicroCAN_Message_t engine_msg = {
    .msg_id  = 0x100,
    .dlc     = 4,
    .sig_num = SIG_ENGINE_COUNT,
};

/* =========================================================
 * 报文 2 — 0x200  车速与加速度
 * ========================================================= */

typedef enum {
    SIG_VEHICLE_SPEED = 0,
    SIG_ACCEL_X,
    SIG_DYNAMICS_COUNT,
} VehicleDynamics_SigIdx_t;

static MicroCAN_Signal_t dynamics_signals[SIG_DYNAMICS_COUNT] = {
    [SIG_VEHICLE_SPEED] = {
        .start_bits = 0,
        .length     = 16,
        .byte_order = MICROCAN_BYTEORDER_MOTOROLA,
        .is_signed  = false,
        .factor     = 0.01f,
        .offset     = 0.0f,
        .Min        = 0.0f,
        .Max        = 300.0f,
        .type       = MICROCAN_SIG_FLOAT,
    },
    [SIG_ACCEL_X] = {
        .start_bits = 16,
        .length     = 16,
        .byte_order = MICROCAN_BYTEORDER_MOTOROLA,
        .is_signed  = true,
        .factor     = 0.001f,
        .offset     = 0.0f,
        .Min        = -20.0f,
        .Max        = 20.0f,
        .type       = MICROCAN_SIG_DOUBLE,
    },
};

static MicroCAN_Message_t dynamics_msg = {
    .msg_id  = 0x200,
    .dlc     = 4,
    .sig_num = SIG_DYNAMICS_COUNT,
};

/* =========================================================
 * 模拟硬件驱动：打印帧内容
 * ========================================================= */
static void HAL_CAN_Transmit_Sim(uint32_t id, const uint8_t *data, uint8_t dlc)
{
    printf("  [TX] ID=0x%03X  DLC=%d  Data:", id, dlc);
    for (int i = 0; i < dlc; i++) {
        printf(" %02X", data[i]);
    }
    printf("\n");
}

/* =========================================================
 * 示例 1：发动机状态报文 — 发送流程
 * ========================================================= */
static MicroCAN_Status_t demo_engine_tx(void)
{
    printf("\n=== [TX] Engine Status (0x100) ===\n");

    /* Set：物理值写入信号，类型须与 .type 字段匹配 */
    uint16_t speed = 3500;          /* rpm，对应 MICROCAN_SIG_U16 */
    int      temp  = 90;            /* °C， 对应 MICROCAN_SIG_INT  */
    uint8_t  mode  = 3;             /* 档位，对应 MICROCAN_SIG_U8   */

    CHECK_OK(MicroCAN_SetSignalValue(&engine_msg, &speed, SIG_ENGINE_SPEED));
    CHECK_OK(MicroCAN_SetSignalValue(&engine_msg, &temp,  SIG_ENGINE_TEMP));
    CHECK_OK(MicroCAN_SetSignalValue(&engine_msg, &mode,  SIG_ENGINE_MODE));

    printf("  Set: speed=%u rpm, temp=%d °C, mode=%u\n", speed, temp, mode);

    /* Pack：物理值 → 原始值 → 编码进 msg_data */
    CHECK_OK(MicroCAN_Pack(&engine_msg));

    /* 送入驱动（使用宏访问报文属性） */
    HAL_CAN_Transmit_Sim(
        MicroCAN_GetMessageId(engine_msg),
        MicroCAN_GetFrameData(engine_msg),
        MicroCAN_GetMessageDLC(engine_msg)
    );

    return MICROCAN_OK;
}

/* =========================================================
 * 示例 2：发动机状态报文 — 接收流程
 * ========================================================= */
static MicroCAN_Status_t demo_engine_rx(void)
{
    printf("\n=== [RX] Engine Status (0x100) ===\n");

    /*
     * 模拟硬件驱动接收到的原始帧数据。
     * 实际项目中这段数据来自 HAL_CAN_GetRxMessage / bxCAN RxFIFO 等。
     */
    const uint8_t raw_data[] = { 0x58, 0x1B, 0x7A, 0x02 };
    const size_t  raw_len    = sizeof(raw_data);

    printf("  Raw: ");
    for (size_t i = 0; i < raw_len; i++) printf("%02X ", raw_data[i]);
    printf("\n");

    /* UnPack：原始数据直接传入，无需手动 memcpy */
    CHECK_OK(MicroCAN_UnPack(&engine_msg, raw_data, raw_len));

    /* Get：从信号读出物理值，类型须与 .type 字段匹配 */
    uint16_t speed_out = 0;
    int      temp_out  = 0;
    uint8_t  mode_out  = 0;

    CHECK_OK(MicroCAN_GetSignalValue(&engine_msg, &speed_out, SIG_ENGINE_SPEED));
    CHECK_OK(MicroCAN_GetSignalValue(&engine_msg, &temp_out,  SIG_ENGINE_TEMP));
    CHECK_OK(MicroCAN_GetSignalValue(&engine_msg, &mode_out,  SIG_ENGINE_MODE));

    printf("  Got: speed=%u rpm, temp=%d °C, mode=%u\n",
           speed_out, temp_out, mode_out);

    return MICROCAN_OK;
}

/* =========================================================
 * 示例 3：车速与加速度 — 发送流程（Motorola 字节序）
 * ========================================================= */
static MicroCAN_Status_t demo_dynamics_tx(void)
{
    printf("\n=== [TX] Vehicle Dynamics (0x200, Motorola) ===\n");

    float  vspeed = 120.5f;         /* km/h，对应 MICROCAN_SIG_FLOAT  */
    double accel  = -3.75;          /* m/s²，对应 MICROCAN_SIG_DOUBLE */

    CHECK_OK(MicroCAN_SetSignalValue(&dynamics_msg, &vspeed, SIG_VEHICLE_SPEED));
    CHECK_OK(MicroCAN_SetSignalValue(&dynamics_msg, &accel,  SIG_ACCEL_X));

    printf("  Set: speed=%.2f km/h, accel_x=%.3f m/s²\n", vspeed, accel);

    CHECK_OK(MicroCAN_Pack(&dynamics_msg));

    HAL_CAN_Transmit_Sim(
        MicroCAN_GetMessageId(dynamics_msg),
        MicroCAN_GetFrameData(dynamics_msg),
        MicroCAN_GetMessageDLC(dynamics_msg)
    );

    return MICROCAN_OK;
}

/* =========================================================
 * 示例 4：车速与加速度 — 接收流程
 * ========================================================= */
static MicroCAN_Status_t demo_dynamics_rx(void)
{
    printf("\n=== [RX] Vehicle Dynamics (0x200, Motorola) ===\n");

    const uint8_t raw_data[] = { 0x2F, 0x0A, 0xFF, 0x13 };
    const size_t  raw_len    = sizeof(raw_data);

    printf("  Raw: ");
    for (size_t i = 0; i < raw_len; i++) printf("%02X ", raw_data[i]);
    printf("\n");

    CHECK_OK(MicroCAN_UnPack(&dynamics_msg, raw_data, raw_len));

    float  vspeed_out = 0.0f;
    double accel_out  = 0.0;

    CHECK_OK(MicroCAN_GetSignalValue(&dynamics_msg, &vspeed_out, SIG_VEHICLE_SPEED));
    CHECK_OK(MicroCAN_GetSignalValue(&dynamics_msg, &accel_out,  SIG_ACCEL_X));

    printf("  Got: speed=%.2f km/h, accel_x=%.3f m/s²\n", vspeed_out, accel_out);

    return MICROCAN_OK;
}

/* =========================================================
 * 示例 5：错误处理
 * ========================================================= */
static void demo_error_handling(void)
{
    printf("\n=== Error Handling ===\n");

    MicroCAN_Status_t sta;

    /* 越界索引 */
    uint16_t dummy = 0;
    sta = MicroCAN_SetSignalValue(&engine_msg, &dummy, 999);
    printf("  Out-of-range index => status %d (expect OVERDLC_ERR=%d)\n",
           sta, MICROCAN_OVERDLC_ERR);

    /* 空指针 */
    sta = MicroCAN_Pack(NULL);
    printf("  NULL msg pointer   => status %d (expect PARAMERR=%d)\n",
           sta, MICROCAN_PARAMERR);

    /* 信号值超上限，Pack 会自动限幅 */
    uint16_t over_speed = 9999;     /* 超过 Max=6500 */
    MicroCAN_SetSignalValue(&engine_msg, &over_speed, SIG_ENGINE_SPEED);
    MicroCAN_Pack(&engine_msg);

    uint16_t clamped = 0;
    MicroCAN_GetSignalValue(&engine_msg, &clamped, SIG_ENGINE_SPEED);
    printf("  Overrange set 9999 => clamped to %u rpm (Max=6500)\n", clamped);
}

/* =========================================================
 * main
 * ========================================================= */
int main(void)
{
    printf("========== MicroCAN Demo ==========\n");

    /* --- 初始化两条报文 --- */
    if (MicroCAN_Init(&engine_msg,   engine_signals)   != MICROCAN_OK ||
        MicroCAN_Init(&dynamics_msg, dynamics_signals) != MICROCAN_OK)
    {
        printf("[FATAL] MicroCAN_Init failed\n");
        return -1;
    }

    /* --- 发送示例 --- */
    demo_engine_tx();
    demo_dynamics_tx();

    /* --- 接收示例 --- */
    demo_engine_rx();
    demo_dynamics_rx();

    /* --- 错误处理示例 --- */
    demo_error_handling();

    printf("\n========== Demo End ==========\n");
    return 0;
}