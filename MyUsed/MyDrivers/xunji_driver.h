#ifndef __XUNJI_DRIVER_H
#define __XUNJI_DRIVER_H

#include "stdint.h"

// 巡迹传感器数量
#define XUNJI_COUNT 5

typedef uint8_t (*Xunji_ReadPin_FP)(void);

// 绑定引脚读取回调
// 成功返回 0，参数非法返回 -1
int Xunji_Driver_Bind(const Xunji_ReadPin_FP *pins, uint8_t count);

// 读取巡迹状态
// 成功返回 0，参数非法或未初始化返回 -1
int Xunji_Get(uint8_t *xunji, uint8_t count);

void Xunji_Init(void);

#endif
