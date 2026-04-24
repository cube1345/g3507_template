#ifndef DEVICE_OPS_H
#define DEVICE_OPS_H

// 开关类设备的通用 ops 接口，他们的操作只有开关反转
typedef struct {
    void (*on)(void *me);
    void (*off)(void *me);
    void (*toggle)(void *me);
} device_switch_ops_t;

#endif
