#ifndef MYUSED_MYDEVICES_ENCODER_DRIVER_H
#define MYUSED_MYDEVICES_ENCODER_DRIVER_H

#include "stdint.h"
#include "device_base.h"
#include "device_ops.h"
#include "platform_gpio.h"

typedef struct encoder_s encoder_t;

// Encoder的操作结构体事实上继承一个通用计数设备操作结构体
typedef device_counter_ops_t encoder_base_ops_t;

// encoder的配置结构体
typedef struct {
    platform_gpio_t phase_a; // A相GPIO配置
    platform_gpio_t phase_b; // B相GPIO配置
    int8_t dir; // 计数方向修正，1为正向，-1为反向
} encoder_cfg_t;

// Encoder设备结构体
struct encoder_s {
    device_base_t base; // 是否初始化？
    const encoder_base_ops_t *ops; // 操作函数指针
    encoder_cfg_t cfg; // 配置参数
    volatile int32_t count; // 编码器计数值
};

void Encoder_InitDevice(encoder_t *me, const encoder_cfg_t *cfg);
int32_t Encoder_GetCount(encoder_t *me);
void Encoder_ResetCount(encoder_t *me);
void Encoder_UpdateFromAB(encoder_t *me,
                          uint8_t changed_a,
                          uint8_t changed_b,
                          uint8_t level_a,
                          uint8_t level_b);
const encoder_base_ops_t *Encoder_GetBaseOps(void);

#endif
