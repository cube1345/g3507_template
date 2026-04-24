#ifndef __LED_DRIVER_H
#define __LED_DRIVER_H

#include "stdint.h"
#include "device_base.h"
#include "device_ops.h"
#include "platform_gpio.h"

typedef struct led_s led_t;

// LED的操作结构体事实上继承一个通用的开关设备操作结构体
typedef device_switch_ops_t led_base_ops_t;

typedef struct {
    platform_gpio_t gpio;
} led_gpio_t;

// led的配置结构体
typedef struct {
    led_gpio_t hw; // 硬件相关配置
    uint8_t active_low; // 是否低电平点亮
    uint8_t default_on; // 是否默认点亮
} led_cfg_t;

// LED设备结构体
struct led_s {
    device_base_t base; // 是否初始化？
    const led_base_ops_t *ops; // 操作函数指针
    led_cfg_t cfg; // 配置参数
};

void LED_Init(led_t *me, const led_cfg_t *cfg);
void LED_On(led_t *me);
void LED_Off(led_t *me);
void LED_Toggle(led_t *me);
const led_base_ops_t *LED_GetBaseOps(void);

#endif
