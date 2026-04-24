#include "bsp_gpio.h"


/**
* @brief 模拟 STM32 HAL_GPIO_ReadPin 的功能
 * @param GPIOx 端口号 (例如 GPIOA, GPIOB)
 * @param GPIO_Pin 引脚掩码 (例如 DL_GPIO_PIN_18)
 * @return uint8_t 0 为低电平, 1 为高电平
 */
uint8_t BSP_GPIO_ReadPin(GPIO_Regs *GPIOx, uint32_t GPIO_Pin)
{
    // 如果返回的位值不为 0，说明该位是 1
    if (DL_GPIO_readPins(GPIOx, GPIO_Pin) & GPIO_Pin) {
        return 1;
    } else {
        return 0;
    }
}

void BSP_GPIO_WritePin(GPIO_Regs *GPIOx, uint32_t GPIO_Pin, uint8_t Value)
{
   if (Value == 0) {
       DL_GPIO_clearPins(GPIOx, GPIO_Pin);
   } else {
       DL_GPIO_setPins(GPIOx, GPIO_Pin);
   }
}

void BSP_GPIO_TogglePin(GPIO_Regs *GPIOx, uint32_t GPIO_Pin)
{
    DL_GPIO_togglePins(GPIOx, GPIO_Pin);
}

void BSP_GPIO_SetPin(GPIO_Regs *GPIOx, uint32_t GPIO_Pin)
{
    DL_GPIO_setPins(GPIOx, GPIO_Pin);
}

void BSP_GPIO_ClearPin(GPIO_Regs *GPIOx, uint32_t GPIO_Pin)
{
    DL_GPIO_clearPins(GPIOx, GPIO_Pin);
}