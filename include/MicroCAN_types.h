/**
 * @file MicroCAN_types.h
 * @author xfp23
 * @brief
 * @version 0.1
 * @date 2026-06-19
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef MICROCAN_TYPES_H
#define MICROCAN_TYPES_H

#include "MicroCAN_Conf.h"
#include "stdint.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    MICROCAN_OK,
    MICROCAN_ERR,
    MICROCAN_PARAMERR,
    MICROCAN_OVERDLC_ERR,
    MICROCAN_BYTEORDER_ERR,
    MICROCAN_BYTELEN_ERR,
} MicroCAN_Status_t;

typedef enum
{
    MICROCAN_MSGSTA_OK,
    MICROCAN_MSGSTA_ERR,
} MicroCAN_MsgSta_t;

typedef enum
{
    MICROCAN_BYTEORDER_INTEL,
    MICROCAN_BYTEORDER_MOTOROLA,
} MicroCAN_ByteOrder_t;

typedef enum
{
    MICROCAN_SIG_U8,
    MICROCAN_SIG_U16,
    MICROCAN_SIG_U32,
    MICROCAN_SIG_INT,
    MICROCAN_SIG_FLOAT,
    MICROCAN_SIG_DOUBLE,
} MicroCAN_SigType_t; // 类型定义

typedef struct
{
    uint16_t start_bits;
    uint16_t length;
    MicroCAN_ByteOrder_t byte_order;
    bool is_signed; // 有无符号
    float factor;   // 精度
    float offset;   // 偏移
    float Min;
    float Max;

    double value;    // 信号的值
    MicroCAN_SigType_t type;
} MicroCAN_Signal_t; // 一条信号所包含的信息

typedef struct
{
    uint32_t msg_id;
    MicroCAN_MsgSta_t msg_sta; 
    uint8_t dlc;
    uint16_t sig_num;
    MicroCAN_Signal_t *signal;
    uint8_t msg_data[8];
} MicroCAN_Message_t; // 一条报文的数据结构



#ifdef __cplusplus
}
#endif
#endif