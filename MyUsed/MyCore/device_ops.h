#ifndef DEVICE_OPS_H
#define DEVICE_OPS_H

#include "stdint.h"

// 开关类设备的通用 ops 接口，他们的操作只有开关反转
typedef struct {
    void (*on)(void *me);
    void (*off)(void *me);
    void (*toggle)(void *me);
} device_switch_ops_t;

// 输入类设备的通用 ops 接口，例如按键、循迹、限位开关等
typedef struct {
    uint8_t (*scan)(void *me);
    uint8_t (*read_level)(void *me);
} device_input_ops_t;

// 计数类设备的通用 ops 接口，例如编码器等
typedef struct {
    int32_t (*get_count)(void *me);
    void (*reset_count)(void *me);
    void (*update_from_ab)(void *me,
                           uint8_t changed_a,
                           uint8_t changed_b,
                           uint8_t level_a,
                           uint8_t level_b);
} device_counter_ops_t;

#endif
