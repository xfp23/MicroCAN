#include "MicroCAN.h"

#define CHECK_PARAM(x)                \
    do                                \
    {                                 \
        if (x == NULL)                \
        {                             \
            return MICROCAN_PARAMERR; \
        }                             \
    } while (0)

MicroCAN_Status_t MicroCAN_Init(MicroCAN_Message_t *msg, MicroCAN_Signal_t *sig, size_t sig_num)
{
    CHECK_PARAM(msg);
    CHECK_PARAM(sig);

    msg->signal = NULL;

    if (msg->dlc <= sig_num)
    {
        return MICROCAN_OVERDLC_ERR;
    }

    msg->signal = sig;

    return MICROCAN_OK;
}

MicroCAN_Status_t MicroCAN_SetSignalValue(MicroCAN_Message_t *msg, void *value, size_t index)
{
    CHECK_PARAM(msg);
    CHECK_PARAM(value);

    if (msg->dlc <= index)
    {
        return MICROCAN_OVERDLC_ERR;
    }

    if (msg->signal == NULL)
    {
        return MICROCAN_ERR;
    }

    switch (msg->signal[index].type)
    {
    case MICROCAN_SIG_U8:
        msg->signal[index].value = *((uint8_t *)(value));
        break;

    case MICROCAN_SIG_U16:
        msg->signal[index].value = *((uint16_t *)(value));
        break;

    case MICROCAN_SIG_U32:
        msg->signal[index].value = *((uint32_t *)(value));
        break;

    case MICROCAN_SIG_INT:
        msg->signal[index].value = *((int *)(value));
        break;

    case MICROCAN_SIG_DOUBLE:
        msg->signal[index].value = *((double *)(value));
        break;

    case MICROCAN_SIG_FLOAT:
        msg->signal[index].value = *((float *)(value));
        break;

    default:

        return MICROCAN_ERR;
    }

    return MICROCAN_OK;
}

MicroCAN_Status_t MicroCAN_GetSignalValue(MicroCAN_Message_t *msg, void *value, size_t index)
{
    CHECK_PARAM(msg);
    CHECK_PARAM(value);

    if (msg->dlc <= index)
    {
        return MICROCAN_OVERDLC_ERR;
    }

    if (msg->signal == NULL)
    {
        return MICROCAN_ERR;
    }

    switch (msg->signal[index].type)
    {
    case MICROCAN_SIG_U8:
        (*((uint8_t *)value)) = (uint8_t)msg->signal[index].value;
        break;

    case MICROCAN_SIG_U16:
        (*((uint16_t *)value)) = (uint16_t)msg->signal[index].value;
        break;

    case MICROCAN_SIG_U32:
        (*((uint32_t *)value)) = (uint32_t)msg->signal[index].value;
        break;

    case MICROCAN_SIG_INT:
        (*((int *)value)) = (int)msg->signal[index].value;
        break;

    case MICROCAN_SIG_DOUBLE:
        (*((double *)value)) = (double)msg->signal[index].value;
        break;

    case MICROCAN_SIG_FLOAT:
        (*((float *)value)) = (float)msg->signal[index].value;
        break;

    default:

        return MICROCAN_ERR;
    }

    return MICROCAN_OK;
}

MicroCAN_Status_t MicroCAN_Pack(MicroCAN_Message_t *msg)
{
    CHECK_PARAM(msg);

    if (msg->signal == NULL)
        return MICROCAN_ERR;

    // 清空数据区
    memset(msg->msg_data, 0, sizeof(msg->msg_data));

    for (uint16_t i = 0; i < msg->sig_num; i++)
    {
        MicroCAN_Signal_t *sig = &msg->signal[i];

        // start_bit越界检查
        if (sig->start_bits >= 64)
            return MICROCAN_BYTELEN_ERR;

        // 信号长度越界检查
        if (sig->length == 0 || sig->length > 64)
            return MICROCAN_BYTELEN_ERR;

        // 限幅
        double val = sig->value;
        if (val > sig->Max)
            val = sig->Max;
        if (val < sig->Min)
            val = sig->Min;

        // 物理值 = 原始值: raw = (value - offset) / factor
        int64_t raw;
        if (sig->factor == 0.0f)
            return MICROCAN_ERR; // 防除零

        if (sig->is_signed)
            raw = (int64_t)((val - sig->offset) / sig->factor);
        else
            raw = (uint64_t)((val - sig->offset) / sig->factor);

        // Intel 小端：start_bit是LSB，从低位往高位连续写
        if (sig->byte_order == MICROCAN_BYTEORDER_INTEL)
        {
            for (uint16_t j = 0; j < sig->length; j++)
            {
                uint16_t bit_pos = sig->start_bits + j; // 当前操作的bit绝对位置
                uint8_t byte_idx = bit_pos / 8;         // 落在哪个字节
                uint8_t bit_idx = bit_pos % 8;          // 字节内第几位

                if (byte_idx >= 8)
                    return MICROCAN_BYTELEN_ERR;

                if ((raw >> j) & 0x01)
                    msg->msg_data[byte_idx] |= (1u << bit_idx);
                else
                    msg->msg_data[byte_idx] &= ~(1u << bit_idx);
            }
        }
        //  Motorola 大端：start_bit是MSB，按DBC标准位序写
        else if (sig->byte_order == MICROCAN_BYTEORDER_MOTOROLA)
        {
            // Motorola模式下，start_bit是MSB的位置（DBC约定）
            // bit序：每行bit7~bit0，行从上到下是byte0~byte7
            // 从MSB开始，往右走到bit0，然后跳到下一行的bit7继续

            uint16_t msb_pos = sig->start_bits; // MSB在Motorola位图中的位置

            for (uint16_t j = 0; j < sig->length; j++)
            {
                // 当前bit在Motorola位图中的位置（MSB是第0个，往后排）
                // Motorola位序转字节索引：
                uint8_t byte_idx = msb_pos / 8;
                uint8_t bit_idx = msb_pos % 8;

                if (byte_idx >= 8)
                    return MICROCAN_BYTELEN_ERR;

                uint8_t raw_bit = (raw >> (sig->length - 1 - j)) & 0x01; // MSB先写

                if (raw_bit)
                    msg->msg_data[byte_idx] |= (1u << bit_idx);
                else
                    msg->msg_data[byte_idx] &= ~(1u << bit_idx);

                // Motorola位位置推进规则：bit_idx-1，到0后跳到下一字节的bit7
                if (bit_idx == 0)
                    msb_pos = (byte_idx + 1) * 8 + 7;
                else
                    msb_pos = msb_pos - 1;
            }
        }
        else
        {
            return MICROCAN_BYTEORDER_ERR;
        }
    }

    return MICROCAN_OK;
}

MicroCAN_Status_t MicroCAN_UnPack(MicroCAN_Message_t *msg)
{
    CHECK_PARAM(msg);
    if (msg->signal == NULL)
        return MICROCAN_ERR;

    for (uint16_t i = 0; i < msg->sig_num; i++)
    {
        MicroCAN_Signal_t *sig = &msg->signal[i];
        uint64_t raw = 0;

        if (sig->byte_order == MICROCAN_BYTEORDER_INTEL)
        {
            for (uint16_t j = 0; j < sig->length; j++)
            {
                uint16_t bit_pos = sig->start_bits + j;
                uint8_t byte_idx = bit_pos / 8;
                uint8_t bit_idx = bit_pos % 8;

                if (byte_idx >= 8)
                    return MICROCAN_BYTELEN_ERR;

                if ((msg->msg_data[byte_idx] >> bit_idx) & 0x01)
                    raw |= (1ULL << j);
            }
        }
        else if (sig->byte_order == MICROCAN_BYTEORDER_MOTOROLA)
        {
            uint16_t msb_pos = sig->start_bits;

            for (uint16_t j = 0; j < sig->length; j++)
            {
                uint8_t byte_idx = msb_pos / 8;
                uint8_t bit_idx = msb_pos % 8;

                if (byte_idx >= 8)
                    return MICROCAN_BYTELEN_ERR;

                if ((msg->msg_data[byte_idx] >> bit_idx) & 0x01)
                    raw |= (1ULL << (sig->length - 1 - j));

                if (bit_idx == 0)
                    msb_pos = (byte_idx + 1) * 8 + 7;
                else
                    msb_pos--;
            }
        }
        else
        {
            return MICROCAN_BYTEORDER_ERR;
        }

        // 有符号扩展
        if (sig->is_signed && (raw >> (sig->length - 1)) & 0x01)
        {
            // 符号位为1，做符号扩展
            int64_t signed_raw = (int64_t)(raw | (~0ULL << sig->length));
            sig->value = (double)signed_raw * sig->factor + sig->offset;
        }
        else
        {
            sig->value = (double)raw * sig->factor + sig->offset;
        }
    }

    return MICROCAN_OK;
}
