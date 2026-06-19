/**
 * @file MicroCAN.h
 * @author xfp23 (https://github.com/xfp23)
 * @brief Lightweight CAN message packing and unpacking library.
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

/**
 * @brief Get the CAN identifier of a message.
 *
 * @param msg Message object.
 * @return CAN message identifier.
 */
#define MicroCAN_GetMessageId(msg) ((msg).msg_id)

/**
 * @brief Get the raw data buffer of a message.
 *
 * @param msg Message object.
 * @return Pointer to the message data buffer.
 */
#define MicroCAN_GetFrameData(msg) ((msg).msg_data)

/**
 * @brief Get the DLC (Data Length Code) of a message.
 *
 * @param msg Message object.
 * @return DLC value of the message.
 */
#define MicroCAN_GetMessageDLC(msg) ((msg).dlc)

/**
 * @brief Initialize a CAN message and register its signals.
 *
 * This function associates a signal table with a CAN message object.
 * The registered signal count shall not exceed the maximum number of
 * signals supported by the message.
 *
 * @param msg Pointer to the message object.
 * @param sig Pointer to the signal table.
 * @param sig_num Number of signals to register.
 *
 * @retval MICROCAN_OK             Initialization completed successfully.
 * @retval MICROCAN_INVALID_PARAM  Invalid input parameter.
 * @retval MICROCAN_ERROR          Initialization failed.
 */
extern MicroCAN_Status_t MicroCAN_Init(MicroCAN_Message_t *msg,
                                       MicroCAN_Signal_t *sig,
                                       size_t sig_num);

/**
 * @brief Set the value of a signal within a CAN message.
 *
 * The input value shall point to a variable whose type matches the
 * signal type definition.
 *
 * @param msg Pointer to the message object.
 * @param value Pointer to the value to be assigned.
 * @param index Signal index within the message.
 *
 * @retval MICROCAN_OK             Signal value updated successfully.
 * @retval MICROCAN_INVALID_PARAM  Invalid input parameter.
 * @retval MICROCAN_INVALID_INDEX  Signal index out of range.
 */
extern MicroCAN_Status_t MicroCAN_SetSignalValue(MicroCAN_Message_t *msg,
                                                 void *value,
                                                 size_t index);

/**
 * @brief Retrieve the value of a signal from a CAN message.
 *
 * The output buffer shall point to a variable whose type matches the
 * signal type definition.
 *
 * @param msg Pointer to the message object.
 * @param value Pointer to the output buffer used to store the signal value.
 * @param index Signal index within the message.
 *
 * @retval MICROCAN_OK             Signal value retrieved successfully.
 * @retval MICROCAN_INVALID_PARAM  Invalid input parameter.
 * @retval MICROCAN_INVALID_INDEX  Signal index out of range.
 */
extern MicroCAN_Status_t MicroCAN_GetSignalValue(const MicroCAN_Message_t *msg,
                                                 void *value,
                                                 size_t index);

/**
 * @brief Pack signal values into the CAN frame data buffer.
 *
 * This function encodes all registered signal values and updates the
 * raw CAN frame payload.
 *
 * @param msg Pointer to the message object.
 *
 * @retval MICROCAN_OK             Packing completed successfully.
 * @retval MICROCAN_INVALID_PARAM  Invalid input parameter.
 * @retval MICROCAN_ERROR          Packing failed.
 */
extern MicroCAN_Status_t MicroCAN_Pack(MicroCAN_Message_t *msg);

/**
 * @brief Unpack the CAN frame data buffer into signal values.
 *
 * This function decodes the raw CAN frame payload and updates all
 * registered signal values.
 *
 * @param msg Pointer to the message object.
 *
 * @retval MICROCAN_OK             Unpacking completed successfully.
 * @retval MICROCAN_INVALID_PARAM  Invalid input parameter.
 * @retval MICROCAN_ERROR          Unpacking failed.
 */
extern MicroCAN_Status_t MicroCAN_UnPack(MicroCAN_Message_t *msg);

#ifdef __cplusplus
}
#endif

#endif