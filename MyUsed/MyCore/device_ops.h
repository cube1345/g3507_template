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

/**
 * @brief 显示类设备通用 ops 接口，例如 OLED。
 */
typedef struct {
    void (*init)(void *me);
    void (*update)(void *me);
    void (*clear)(void *me);
} device_display_ops_t;

/**
 * @brief 传感器类设备通用 ops 接口，例如 IMU。
 */
typedef struct {
    int32_t (*init)(void *me);
    int32_t (*read_raw)(void *me, void *raw_data);
    int32_t (*read_scaled)(void *me, void *scaled_data);
} device_sensor_ops_t;

/**
 * @brief 多路输入阵列通用 ops 接口，例如五路循迹模块。
 */
typedef struct {
    int32_t (*read_all)(void *me, uint8_t *data, uint8_t count);
} device_array_input_ops_t;

/**
 * @brief 直流电机类设备通用 ops 接口。
 */
typedef struct {
    void (*start)(void *me);
    void (*stop)(void *me);
    void (*set_pwm)(void *me, int32_t pwm_a, int32_t pwm_b);
    int32_t (*velocity_calc)(void *me, uint8_t channel, int32_t target, int32_t current);
} device_motor_ops_t;

/**
 * @brief 步进电机类设备通用 ops 接口。
 */
typedef struct {
    void (*enable)(void *me, uint8_t enable);
    void (*set_direction)(void *me, uint8_t direction);
    uint8_t (*set_frequency)(void *me, uint32_t frequency_hz);
    void (*start)(void *me);
    void (*stop)(void *me);
    void (*service)(void *me);
} device_stepper_ops_t;

#endif
