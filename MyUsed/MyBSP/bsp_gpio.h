#ifndef  __BSP_GPIO_H
#define  __BSP_GPIO_H

#include "ti_msp_dl_config.h"

uint8_t BSP_GPIO_ReadPin(GPIO_Regs *GPIOx, uint32_t GPIO_Pin);
void BSP_GPIO_WritePin(GPIO_Regs *GPIOx, uint32_t GPIO_Pin, uint8_t Value);
void BSP_GPIO_TogglePin(GPIO_Regs *GPIOx, uint32_t GPIO_Pin);
void BSP_GPIO_SetPin(GPIO_Regs *GPIOx, uint32_t GPIO_Pin);
void BSP_GPIO_ClearPin(GPIO_Regs *GPIOx, uint32_t GPIO_Pin);

#endif
