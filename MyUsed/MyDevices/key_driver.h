#ifndef __KEY_DRIVER_H
#define __KEY_DRIVER_H

#include "stdint.h"
#include "device_base.h"
#include "device_ops.h"
#include "platform_gpio.h"

#define DEBOUNCE_TIME        20
#define LONG_PRESS_TIME      500
#define CONTINUOUS_START     1000
#define CONTINUOUS_INTERVAL  100

typedef struct key_s key_t;

// Key的操作结构体事实上继承一个通用输入设备操作结构体
typedef device_input_ops_t key_base_ops_t;

// 获取系统tick的函数指针，单位ms
typedef uint32_t (*key_get_tick_t)(void);

// Key状态机状态
typedef enum {
    KEY_STATE_RELEASE,
    KEY_STATE_DEBOUNCE,
    KEY_STATE_PRESSED,
    KEY_STATE_LONG_PRESS,
    KEY_STATE_CONTINUOUS
} Key_State;

// Key事件值，高4位表示按键索引，低4位表示事件类型
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

// key的配置结构体
typedef struct {
    platform_gpio_t gpio; // 硬件GPIO配置
    uint8_t active_low; // 是否低电平按下
    key_get_tick_t get_tick; // 获取系统tick的函数
} key_cfg_t;

// Key设备结构体
struct key_s {
    device_base_t base; // 是否初始化？
    const key_base_ops_t *ops; // 操作函数指针
    key_cfg_t cfg; // 配置参数
    Key_State state; // 按键状态机状态
    uint32_t last_time; // 上一次状态切换时间
    uint32_t continuous_time; // 连发计时时间
};

void Key_InitDevice(key_t *me, const key_cfg_t *cfg);
uint8_t Key_Scan(key_t *me);
uint8_t Key_ReadLevel(key_t *me);
const key_base_ops_t *Key_GetBaseOps(void);

#endif
