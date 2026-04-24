#ifndef __KEY_DRIVER_H
#define __KEY_DRIVER_H

#include "stdint.h"

// ============================== 
// 按键时序参数配置（单位：ms）
// ==============================
// 去抖时间：按下后至少稳定这么久，才认为是有效按下
#define DEBOUNCE_TIME        20
// 长按判定阈值：按下持续超过该值触发 KEYx_LONG
#define LONG_PRESS_TIME      500
// 连发起始阈值：进入长按后继续保持这么久进入连发态
#define CONTINUOUS_START     1000
// 连发周期：进入连发态后，每隔该时间触发一次 KEYx_CONTINUOUS
#define CONTINUOUS_INTERVAL  100

// 板级提供的引脚读取函数指针（返回 0/1）
typedef uint8_t (*Key_ReadPin_FP)(void);
// 板级提供的系统时间函数指针（单位 ms）
typedef uint32_t (*Key_GetTick_FP)(void);

// 单个按键的状态机状态
typedef enum {
    KEY_STATE_RELEASE,       // 空闲/释放态
    KEY_STATE_DEBOUNCE,      // 去抖态
    KEY_STATE_PRESSED,       // 已确认按下态
    KEY_STATE_LONG_PRESS,    // 长按态（尚未进入连发）
    KEY_STATE_CONTINUOUS     // 连发态
} Key_State;

// 单个按键驱动对象
// ReadPin/GetTick 由接口层绑定
// 其余字段由驱动层状态机维护
typedef struct {
    Key_ReadPin_FP ReadPin;
    Key_GetTick_FP GetTick;
    Key_State State;
    uint32_t LastTime;
    uint32_t ContinuousTime;
} Key_Driver_t;

// 对外事件编码：低 4 位表示事件类型，高 4 位表示按键索引
// 例如：0x12 = KEY1_SHORT_RELEASE
// 说明：保持你原先定义不变，避免影响业务层映射逻辑
typedef enum {
    KEY_NONE = 0x00,

    KEY0_SHORT = 0x01,
    KEY1_SHORT = 0x11,
    KEY2_SHORT = 0x21,
    KEY3_SHORT = 0x31,
    KEY4_SHORT = 0x41,
    KEY5_SHORT = 0x51,
    KEY6_SHORT = 0x61,
    KEY7_SHORT = 0x71,
    KEY8_SHORT = 0x81,

    KEY0_SHORT_RELEASE = 0x02,
    KEY1_SHORT_RELEASE = 0x12,
    KEY2_SHORT_RELEASE = 0x22,
    KEY3_SHORT_RELEASE = 0x32,
    KEY4_SHORT_RELEASE = 0x42,
    KEY5_SHORT_RELEASE = 0x52,
    KEY6_SHORT_RELEASE = 0x62,
    KEY7_SHORT_RELEASE = 0x72,
    KEY8_SHORT_RELEASE = 0x82,

    KEY0_LONG = 0x03,
    KEY1_LONG = 0x13,
    KEY2_LONG = 0x23,
    KEY3_LONG = 0x33,
    KEY4_LONG = 0x43,
    KEY5_LONG = 0x53,
    KEY6_LONG = 0x63,
    KEY7_LONG = 0x73,
    KEY8_LONG = 0x83,

    KEY0_LONG_RELEASE = 0x04,
    KEY1_LONG_RELEASE = 0x14,
    KEY2_LONG_RELEASE = 0x24,
    KEY3_LONG_RELEASE = 0x34,
    KEY4_LONG_RELEASE = 0x44,
    KEY5_LONG_RELEASE = 0x54,
    KEY6_LONG_RELEASE = 0x64,
    KEY7_LONG_RELEASE = 0x74,
    KEY8_LONG_RELEASE = 0x84,

    KEY0_CONTINUOUS = 0x05,
    KEY1_CONTINUOUS = 0x15,
    KEY2_CONTINUOUS = 0x25,
    KEY3_CONTINUOUS = 0x35,
    KEY4_CONTINUOUS = 0x45,
    KEY5_CONTINUOUS = 0x55,
    KEY6_CONTINUOUS = 0x65,
    KEY7_CONTINUOUS = 0x75,
    KEY8_CONTINUOUS = 0x85,
} Key_Value;

// 接口层调用：绑定按键对象数组并初始化状态机字段
void Key_Driver_Bind(Key_Driver_t *keys, uint8_t num);

// 单个按键状态机扫描（核心逻辑）
uint8_t Key_Scan(Key_Driver_t *key);

// 业务层对外初始化入口（内部转调接口层）
void Key_Init(void);

// 业务层对外获取一次按键事件
Key_Value Key_GetValue(void);

#endif
