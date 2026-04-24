#ifndef __BUZZER_DRIVER_H
#define __BUZZER_DRIVER_H

#include "stdint.h"
#include "device_base.h"
#include "device_ops.h"
#include "platform_gpio.h"

typedef struct buzzer_s buzzer_t;

// 蜂鸣器的操作结构体事实上继承一个通用的开关设备操作结构体
typedef device_switch_ops_t buzzer_base_ops_t;

typedef struct {
    platform_gpio_t gpio;
} buzzer_gpio_t;

// buzzer的配置结构体
typedef struct {
    buzzer_gpio_t hw; // 硬件相关配置
    uint8_t active_high; // 是否高电平响
    uint8_t default_on; // 是否默认响
} buzzer_cfg_t;

// Buzzer设备结构体
struct buzzer_s {
    device_base_t base; // 是否初始化？
    const buzzer_base_ops_t *ops; // 操作函数指针
    buzzer_cfg_t cfg; // 配置参数
};

void Buzzer_InitDevice(buzzer_t *me, const buzzer_cfg_t *cfg);
void Buzzer_On(buzzer_t *me);
void Buzzer_Off(buzzer_t *me);
const buzzer_base_ops_t *Buzzer_GetBaseOps(void);

#endif
