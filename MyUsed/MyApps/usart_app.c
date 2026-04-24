#include "usart_app.h"

/* 保存最近一次解析成功的结果 */
static USART_ParseResult_t g_usart_result;

/*
 * 事件锁存标志。
 * 当解析到事件帧时，对应标志会被置 1，
 * 并一直保持，直到用户代码主动调用对应的 Consume 接口清除。
 */
static volatile uint8_t g_track_found_latched;
static volatile uint8_t g_track_lost_latched;
static volatile uint8_t g_right_angle_latched;
static volatile uint8_t g_qrcode2_latched;
static volatile uint8_t g_qrcode3_latched;

/* 当前接收进度，范围 0~5 */
static uint8_t g_rx_index;

/* 固定长度 5 字节接收缓存:
 * [0] 帧头 0xEE
 * [1] 命令字
 * [2] 数据1
 * [3] 数据2
 * [4] 帧尾 0xFF
 */
static uint8_t g_rx_buffer[5];

static uint8_t USART_App_ConsumeLatchedFlag(volatile uint8_t *flag)
{
    uint32_t primask = __get_PRIMASK();
    uint8_t value;

    __disable_irq();
    value = *flag;
    *flag = 0U;
    if (primask == 0U)
    {
        __enable_irq();
    }

    return value;
}

/* 接收状态复位，只清下标，不清结果 */
static void USART_App_ResetRxState(void)
{
    g_rx_index = 0;
}

/*
 * 根据命令字和数据内容，把原始帧转换成业务消息类型
 * 如果命令字合法但数据内容不符合约定，也视为无效帧
 */
static USART_MsgType_t USART_App_DecodeFrame(uint8_t cmd, uint8_t data1, uint8_t data2)
{
    switch (cmd)
    {
        /* 直线循迹:
         * data1 = 像素偏差
         * data2 = 角度偏差
         */
        case USART_CMD_STRAIGHT_TRACK:
            return USART_MSG_STRAIGHT_TRACK;

        /* 弯道循迹:
         * data1 = 像素偏差
         * data2 = 角度偏差
         */
        case USART_CMD_ARC_TRACK:
            return USART_MSG_ARC_TRACK;

        /* 检测到赛道: 后两个字节固定为 0xAA 0xAA */
        case USART_CMD_TRACK_FOUND:
            if ((data1 == 0xAAU) && (data2 == 0xAAU))
            {
                return USART_MSG_TRACK_FOUND;
            }
            break;

        /* 检测到脱离赛道: 后两个字节固定为 0xAA 0xAA */
        case USART_CMD_TRACK_LOST:
            if ((data1 == 0xAAU) && (data2 == 0xAAU))
            {
                return USART_MSG_TRACK_LOST;
            }
            break;

        /* 检测到直角: 后两个字节固定为 0xAA 0xAA */
        case USART_CMD_RIGHT_ANGLE:
            if ((data1 == 0xAAU) && (data2 == 0xAAU))
            {
                return USART_MSG_RIGHT_ANGLE;
            }
            break;

        /* 二维码识别:
         * 0x22 0x22 -> 二维码2
         * 0x33 0x33 -> 二维码3
         */
        case USART_CMD_QRCODE:
            if ((data1 == 0x22U) && (data2 == 0x22U))
            {
                return USART_MSG_QRCODE_2;
            }

            if ((data1 == 0x33U) && (data2 == 0x33U))
            {
                return USART_MSG_QRCODE_3;
            }
            break;

        default:
            break;
    }

    return USART_MSG_INVALID;
}

/*
 * 一帧完整数据校验通过后，保存到解析结果结构体
 * 对于弯道循迹帧，会把 data1/data2 同时解释为有符号偏差量
 */
static void USART_App_SaveFrame(void)
{
    g_usart_result.raw[0] = g_rx_buffer[0];
    g_usart_result.raw[1] = g_rx_buffer[1];
    g_usart_result.raw[2] = g_rx_buffer[2];
    g_usart_result.raw[3] = g_rx_buffer[3];
    g_usart_result.raw[4] = g_rx_buffer[4];

    g_usart_result.cmd = g_rx_buffer[1];
    g_usart_result.data1 = g_rx_buffer[2];
    g_usart_result.data2 = g_rx_buffer[3];
    g_usart_result.pixel_error = (int8_t) g_rx_buffer[2];
    g_usart_result.angle_error = (int8_t) g_rx_buffer[3];

    /* 结合命令字和数据内容确定消息类型 */
    g_usart_result.type = USART_App_DecodeFrame(
        g_usart_result.cmd,
        g_usart_result.data1,
        g_usart_result.data2
    );

    /*
     * 在这里锁存事件帧。
     * 这样即使后面立刻收到新的直线/弯道循迹帧，
     * 这些事件也不会因为“最新帧被覆盖”而丢失。
     */
    switch (g_usart_result.type)
    {
        case USART_MSG_TRACK_FOUND:
            g_track_found_latched = 1U;
            break;

        case USART_MSG_TRACK_LOST:
            g_track_lost_latched = 1U;
            break;

        case USART_MSG_RIGHT_ANGLE:
            g_right_angle_latched = 1U;
            break;

        case USART_MSG_QRCODE_2:
            g_qrcode2_latched = 1U;
            break;

        case USART_MSG_QRCODE_3:
            g_qrcode3_latched = 1U;
            break;

        default:
            break;
    }

    /* 只有解析成功的有效帧才会拉起 updated 标志 */
    g_usart_result.updated = (g_usart_result.type != USART_MSG_INVALID);
}

void USART_App_Init(void)
{
    /* 初始化解析结果为全 0 / 无消息状态 */
    g_usart_result.type = USART_MSG_NONE;
    g_usart_result.raw[0] = 0;
    g_usart_result.raw[1] = 0;
    g_usart_result.raw[2] = 0;
    g_usart_result.raw[3] = 0;
    g_usart_result.raw[4] = 0;
    g_usart_result.cmd = 0;
    g_usart_result.data1 = 0;
    g_usart_result.data2 = 0;
    g_usart_result.pixel_error = 0;
    g_usart_result.angle_error = 0;
    g_usart_result.updated = 0;
    g_track_found_latched = 0;
    g_track_lost_latched = 0;
    g_right_angle_latched = 0;
    g_qrcode2_latched = 0;
    g_qrcode3_latched = 0;

    USART_App_ResetRxState();
}

/*
 * 串口字节流状态机
 *
 * 工作流程:
 * 1. 等待帧头 0xEE
 * 2. 收满 5 个字节
 * 3. 检查最后 1 字节是否为帧尾 0xFF
 * 4. 若合法则保存结果，否则丢弃本帧
 *
 * 注意:
 * 由于 data1/data2 也可能合法地等于 0xEE，
 * 因此这里不会在收帧过程中遇到 0xEE 就强制重同步。
 * 当前策略是:
 * 1. 只在空闲状态等待帧头 0xEE
 * 2. 一旦收到帧头，就固定再接收后续 4 个字节
 * 3. 收满 5 字节后统一检查帧尾和内容是否合法
 */
uint8_t USART_App_ParseByte(uint8_t rx_byte)
{
    /* 空闲状态下，只接受帧头 */
    if (g_rx_index == 0U)
    {
        if (rx_byte == USART_FRAME_HEAD)
        {
            g_rx_buffer[g_rx_index++] = rx_byte;
        }
        return 0;
    }

    /* 缓存当前字节 */
    g_rx_buffer[g_rx_index++] = rx_byte;

    /* 未收满 5 字节，继续等待 */
    if (g_rx_index < 5U)
    {
        return 0;
    }

    /* 收满后检查帧尾，不合法则丢弃本帧 */
    if (g_rx_buffer[4] != USART_FRAME_TAIL)
    {
        USART_App_ResetRxState();
        return 0;
    }

    /* 合法帧，保存解析结果 */
    USART_App_SaveFrame();
    USART_App_ResetRxState();

    return g_usart_result.updated;
}

/* 返回最近一次解析结果的只读指针 */
const USART_ParseResult_t* USART_App_GetResult(void)
{
    return &g_usart_result;
}

/* 查询是否收到新的有效帧 */
uint8_t USART_App_HasNewFrame(void)
{
    return g_usart_result.updated;
}

USART_MsgType_t USART_App_GetMsgType(void)
{
    return g_usart_result.type;
}

int8_t USART_App_GetPixelError(void)
{
    return g_usart_result.pixel_error;
}

int8_t USART_App_GetAngleError(void)
{
    return g_usart_result.angle_error;
}

uint8_t USART_App_IsStraightTrack(void)
{
    return (g_usart_result.type == USART_MSG_STRAIGHT_TRACK);
}

uint8_t USART_App_IsArcTrack(void)
{
    return (g_usart_result.type == USART_MSG_ARC_TRACK);
}

uint8_t USART_App_IsTrackFound(void)
{
    /* 只查询，不清除锁存标志。 */
    return g_track_found_latched;
}

uint8_t USART_App_IsTrackLost(void)
{
    /* 只查询，不清除锁存标志。 */
    return g_track_lost_latched;
}

uint8_t USART_App_IsRightAngle(void)
{
    /* 只查询，不清除锁存标志。 */
    return g_right_angle_latched;
}

uint8_t USART_App_IsQrCode2(void)
{
    /* 只查询，不清除锁存标志。 */
    return g_qrcode2_latched;
}

uint8_t USART_App_IsQrCode3(void)
{
    /* 只查询，不清除锁存标志。 */
    return g_qrcode3_latched;
}

uint8_t USART_App_ConsumeTrackFound(void)
{
    return USART_App_ConsumeLatchedFlag(&g_track_found_latched);
}

uint8_t USART_App_ConsumeTrackLost(void)
{
    return USART_App_ConsumeLatchedFlag(&g_track_lost_latched);
}

uint8_t USART_App_ConsumeRightAngle(void)
{
    return USART_App_ConsumeLatchedFlag(&g_right_angle_latched);
}

uint8_t USART_App_ConsumeQrCode2(void)
{
    return USART_App_ConsumeLatchedFlag(&g_qrcode2_latched);
}

uint8_t USART_App_ConsumeQrCode3(void)
{
    return USART_App_ConsumeLatchedFlag(&g_qrcode3_latched);
}

/* 用户处理完数据后可手动清标志 */
void USART_App_ClearFlag(void)
{
    g_usart_result.updated = 0;
}

uint8_t USART_App_FetchResult(USART_ParseResult_t* out_result)
{
    if ((out_result == 0) || (g_usart_result.updated == 0U))
    {
        return 0;
    }

    *out_result = g_usart_result;
    g_usart_result.updated = 0;

    return 1;
}

/*
 * UART0 接收中断:
 * 1. 把 RX FIFO 中当前可读的字节全部取出
 * 2. 每个字节都送入协议解析状态机
 *
 * 注意:
 * 中断里不做 printf 之类的耗时操作，避免阻塞接收链路。
 */
void UART_0_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_0_INST)) {
        case DL_UART_MAIN_IIDX_RX:
        {
            while (!DL_UART_Main_isRXFIFOEmpty(UART_0_INST))
            {
                uint8_t rx_byte = DL_UART_Main_receiveData(UART_0_INST);
                USART_App_ParseByte(rx_byte);
            }
            break;
        }
        default:
            break;
    }
}
