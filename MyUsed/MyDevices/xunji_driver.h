#ifndef MYUSED_MYDEVICES_XUNJI_DRIVER_H
#define MYUSED_MYDEVICES_XUNJI_DRIVER_H

/**
 * @file xunji_driver.h
 * @brief 五路循迹模块 Device 层对象封装。
 */

#include <stdint.h>
#include "device_base.h"
#include "device_ops.h"

#define XUNJI_COUNT 5U

typedef struct xunji_s xunji_t;
typedef device_array_input_ops_t xunji_base_ops_t;
typedef uint8_t (*Xunji_ReadPin_FP)(void);

typedef struct {
    Xunji_ReadPin_FP pins[XUNJI_COUNT];
    uint8_t active_high;
} xunji_cfg_t;

struct xunji_s {
    device_base_t base;
    const xunji_base_ops_t *ops;
    xunji_cfg_t cfg;
};

void Xunji_InitDevice(xunji_t *me, const xunji_cfg_t *cfg);
int32_t Xunji_ReadAll(xunji_t *me, uint8_t *xunji, uint8_t count);
const xunji_base_ops_t *Xunji_GetBaseOps(void);

int Xunji_Driver_Bind(const Xunji_ReadPin_FP *pins, uint8_t count);
int Xunji_Get(uint8_t *xunji, uint8_t count);
void Xunji_Init(void);

#endif /* MYUSED_MYDEVICES_XUNJI_DRIVER_H */
