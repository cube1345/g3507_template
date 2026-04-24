#ifndef __USART_APP_H
#define __USART_APP_H

#include <stdint.h>
#include "ti_msp_dl_config.h"

/*
 * 串口协议固定格式:
 * 帧头    命令字    数据1       数据2       帧尾
 * 0xEE    cmd      payload_1   payload_2   0xFF
 *
 * 已约定的协议示例:
 * 直线循迹      0xEE 0x11 像素偏差 角度偏差 0xFF
 * 弯道循迹      0xEE 0x22 像素偏差 角度偏差 0xFF
 * 检测到赛道    0xEE 0x33 0xAA     0xAA     0xFF
 * 检测到脱赛道  0xEE 0x44 0xAA     0xAA     0xFF
 * 检测到直角    0xEE 0x55 0xAA     0xAA     0xFF
 * 二维码2       0xEE 0x66 0x22     0x22     0xFF
 * 二维码3       0xEE 0x66 0x33     0x33     0xFF
 */

#define USART_FRAME_HEAD             (0xEEU)
#define USART_FRAME_TAIL             (0xFFU)

/* 直线循迹命令字 */
#define USART_CMD_STRAIGHT_TRACK     (0x11U)

/* 命令字定义 */
#define USART_CMD_ARC_TRACK          (0x22U)
#define USART_CMD_TRACK_FOUND        (0x33U)
#define USART_CMD_TRACK_LOST         (0x44U)
#define USART_CMD_RIGHT_ANGLE        (0x55U)
#define USART_CMD_QRCODE             (0x66U)

/* 解析后的消息类型 */
typedef enum
{
    USART_MSG_NONE = 0,
    USART_MSG_STRAIGHT_TRACK,
    USART_MSG_ARC_TRACK,
    USART_MSG_TRACK_FOUND,
    USART_MSG_TRACK_LOST,
    USART_MSG_RIGHT_ANGLE,
    USART_MSG_QRCODE_2,
    USART_MSG_QRCODE_3,
    USART_MSG_INVALID
} USART_MsgType_t;

/*
 * 串口解析结果:
 * 1. raw[]     保存完整原始帧，便于调试打印
 * 2. cmd       保存命令字
 * 3. data1/2   保存原始数据区
 * 4. pixel_error / angle_error
 *    仅在 USART_MSG_ARC_TRACK 类型下有明确业务含义
 * 5. updated   置 1 表示收到一帧新的有效数据，用户读取后可手动清零
 */
typedef struct
{
    USART_MsgType_t type;
    uint8_t raw[5];
    uint8_t cmd;
    uint8_t data1;
    uint8_t data2;
    int8_t pixel_error;
    int8_t angle_error;
    uint8_t updated;
} USART_ParseResult_t;

/* 初始化解析模块，建议在系统初始化阶段调用一次 */
void USART_App_Init(void);

/*
 * 向解析器输入 1 个串口字节
 * 返回值:
 * 0: 当前还未拼成完整有效帧
 * 1: 已经成功解析出 1 帧有效数据
 */
uint8_t USART_App_ParseByte(uint8_t rx_byte);

/* 获取最近一次成功解析出的结果结构体指针 */
const USART_ParseResult_t* USART_App_GetResult(void);

/* 查询是否收到新的有效帧 */
uint8_t USART_App_HasNewFrame(void);

/* 清除“新帧到达”标志 */
void USART_App_ClearFlag(void);

/* 获取最近一次解析到的消息类型 */
USART_MsgType_t USART_App_GetMsgType(void);

/* 获取最近一次解析到的像素偏差 */
int8_t USART_App_GetPixelError(void);

/* 获取最近一次解析到的角度偏差 */
int8_t USART_App_GetAngleError(void);

/* 判断最近一次消息是否为直线循迹 */
uint8_t USART_App_IsStraightTrack(void);

/* 判断最近一次消息是否为弯道循迹 */
uint8_t USART_App_IsArcTrack(void);

/* 判断最近一次消息是否为检测到赛道 */
uint8_t USART_App_IsTrackFound(void);

/* 判断最近一次消息是否为检测到脱离赛道 */
uint8_t USART_App_IsTrackLost(void);

/* 判断最近一次消息是否为检测到直角 */
uint8_t USART_App_IsRightAngle(void);

/* 判断最近一次消息是否为二维码2 */
uint8_t USART_App_IsQrCode2(void);

/* 判断最近一次消息是否为二维码3 */
uint8_t USART_App_IsQrCode3(void);

/*
 * 事件锁存接口:
 * 这些接口用于“事件型数据帧”，而不是连续发送的循迹帧。
 * 例如:
 * 1. 检测到赛道
 * 2. 脱离赛道
 * 3. 检测到直角
 * 4. 二维码 2 / 二维码 3
 *
 * 设计原因:
 * 解析器默认只保留“最近一帧”。
 * 如果事件帧刚收到，后面立刻又来了直线/弯道循迹帧，
 * 那么最近一帧就可能被新的循迹帧覆盖，导致用户代码错过这个事件。
 * 因此这里把事件单独做成“锁存标志”，直到用户主动读取清除为止。
 *
 * Consume 行为:
 * 1. 返回 1: 说明自上次读取后，这个事件至少出现过一次
 * 2. 返回 0: 当前没有等待处理的事件
 * 3. 调用后会自动清除对应的锁存标志
 */
uint8_t USART_App_ConsumeTrackFound(void);
uint8_t USART_App_ConsumeTrackLost(void);
uint8_t USART_App_ConsumeRightAngle(void);
uint8_t USART_App_ConsumeQrCode2(void);
uint8_t USART_App_ConsumeQrCode3(void);

/*
 * 读取并清除新帧标志
 * 若返回 1，则 out_result 中带回本次最新解析结果
 * 若返回 0，则表示当前没有新的有效帧
 */
uint8_t USART_App_FetchResult(USART_ParseResult_t* out_result);

/* UART0 串口中断服务函数，负责接收并解析协议帧 */
void UART_0_INST_IRQHandler(void);

#endif
