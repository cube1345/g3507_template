#include "key_int.h"
#include "key_driver.h"
#include "ti_msp_dl_config.h"
#include "bsp_gpio.h"
#include "my_system_lib.h"

// 当前板级接入的按键数量
#define KEY_NUM 3

// 板级按键对象数组，仅在接口层维护
static Key_Driver_t keys[KEY_NUM];

// Key0 读引脚：按下为低电平
static uint8_t Key0_ReadPin(void)
{
    return BSP_GPIO_ReadPin(KEY_NUM0_PORT, KEY_NUM0_PIN);
}

// Key1 读引脚：当前硬件极性与其余不同，这里做一次反相
static uint8_t Key1_ReadPin(void)
{
    return (uint8_t)!BSP_GPIO_ReadPin(KEY_NUM1_PORT, KEY_NUM1_PIN);
}

// Key2 读引脚：按下为低电平
static uint8_t Key2_ReadPin(void)
{
    return BSP_GPIO_ReadPin(KEY_NUM2_PORT, KEY_NUM2_PIN);
}

// 绑定 driver 层所需的时间基准函数（ms）
static uint32_t Key_GetTick(void)
{
    return My_GetTick();
}

// 接口层初始化：只做板级绑定，不做扫描逻辑
void Key_Int_Init(void)
{
    keys[0] = (Key_Driver_t){
        .ReadPin = Key0_ReadPin,
        .GetTick = Key_GetTick,
    };

    keys[1] = (Key_Driver_t){
        .ReadPin = Key1_ReadPin,
        .GetTick = Key_GetTick,
    };

    keys[2] = (Key_Driver_t){
        .ReadPin = Key2_ReadPin,
        .GetTick = Key_GetTick,
    };

    Key_Driver_Bind(keys, KEY_NUM);
}
