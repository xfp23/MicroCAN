/**
 * @file MicroCAN.h
 * @author https://github.com/xfp23
 * @brief CAN报文解析库
 * @version 0.1
 * @date 2026-06-19
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef MICROCAN_H
#define MICROCAN_H

#include "MicroCAN_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MicroCAN_GetMessageId(msg) (msg.msg_id)

#define MicroCAN_GetFrameData(msg) (msg.msg_data)


/**
 * @brief 初始化一条报文，就是把信号注册到报文里
 * 
 * @param msg 报文
 * @param sig 信号数组
 * @param sig_num 要注册进报文的信号数量 此信号数量不能大于报文自身的dlc
 * @return MicroCAN_Status_t 
 */
extern MicroCAN_Status_t MicroCAN_Init(MicroCAN_Message_t *msg,MicroCAN_Signal_t *sig,size_t sig_num);

/**
 * @brief 给报文里的信号给值
 * 
 * @param msg 报文
 * @param value 要给的值
 * @param index 访问码
 * @return MicroCAN_Status_t 
 */
extern MicroCAN_Status_t MicroCAN_SetSignalValue(MicroCAN_Message_t *msg,void *value,size_t index);

/**
 * @brief 获取报文信号的值
 * 
 * @param msg 报文
 * @param value 获取的buffer
 * @param index 访问码
 * @return MicroCAN_Status_t 
 */
extern MicroCAN_Status_t MicroCAN_GetSignalValue(MicroCAN_Message_t *msg,void *value,size_t index);

/**
 * @brief 报文打包
 * 
 * @param msg 报文
 * @return MicroCAN_Status_t 
 */
extern MicroCAN_Status_t MicroCAN_Pack(MicroCAN_Message_t *msg);

/**
 * @brief 报文解包
 * 
 * @param msg 报文
 * @return MicroCAN_Status_t 
 */
extern MicroCAN_Status_t MicroCAN_UnPack(MicroCAN_Message_t *msg);

#ifdef __cplusplus
}
#endif

#endif