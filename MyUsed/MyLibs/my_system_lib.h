#ifndef __MY_SYSTEM_LIB_H
#define __MY_SYSTEM_LIB_H

#include <stdint.h>
#include <stdbool.h>
#include "stdio.h"
#include "ti_msp_dl_config.h"

uint32_t My_GetTick(void);
uint64_t My_GetTimeUs(void);
void My_Delay_ms(uint32_t ms);


void delay_us(uint32_t us);
void delay_ns(uint32_t ns);
void delay_ms(uint32_t ms);

bool DebugUart2_WriteChar(uint8_t ch);
bool DebugUart2_WriteBuffer(const uint8_t *data, uint32_t length);
void DebugUart2_WriteString(const char *str);
void DebugUart2_WriteJustFloatFrame(const float *values, uint32_t count);



uint8_t MY_GPIO_ReadPin(GPIO_Regs *GPIOx, uint32_t GPIO_Pin);


//一些出现之前出现的问题
//在sysconfig里改变引脚名称后要引脚的各种配置会恢复成默认的 要重新配置
//中断尽可能在其他初始化完成后再调用 不然可能出现一些奇怪的问题
//陀螺仪要保证完全静止水平初始化
//中断里不要放耗时多的功能比如printf


#endif
