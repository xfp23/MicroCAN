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
    MICROCAN_MSGSTA_OK,
    MICROCAN_MSGSTA_ERR,
}MicroCAN_MsgSta_t;

typedef enum 
{
    MICROCAN_BYTEORDER_INTEL,
    MICROCAN_BYTEORDER_MOTOROLA,
}MicroCAN_ByteOrder_t;

typedef struct
{
    uint16_t start_bits;
    uint16_t length;
    MicroCAN_ByteOrder_t byte_order;
    bool is_signed; // 有无符号
    float factor; // 精度
    float offset; // 偏移
    float Min;
    float Max;

    double value; // 信号的值
}MicroCAN_Signal_t; // 一条信号所包含的信息

typedef struct 
{
    uint32_t msg_id;
    MicroCAN_MsgSta_t msg_sta;
    uint8_t dlc;
    uint16_t sig_num;
    MicroCAN_Signal_t *signal; // 难点，如果我想达成MicroCAN_CANPack(MicroCAN_Message_t*); 用户在外面可以自由赋值信号的话 msg.xxx_sig.value = xxx;但是好像做不到统一API达成这种的效果
    uint8_t msg_data[8];
}MicroCAN_Message_t; // 一条报文的数据结构

#ifdef __cplusplus
}
#endif
#endif