#ifndef __OLED_INT_H__
#define __OLED_INT_H__

#include "OLED_driver.h"
#include "ti_msp_dl_config.h"

void OLED_Driver_Init(void);

// 使用示例
// 第一步 在OLED_app.c中并绑定好OLED的I2C函数
// 第二步 OLED_Init();
// 第三步 OLED_Printf()或其他屏幕显示函数
// 第四步 OLED_Update()更新屏幕

#endif