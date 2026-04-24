#include "xunji_int.h"
#include "bsp_gpio.h"

static uint8_t Xunji0_ReadPin(void)
{
    return BSP_GPIO_ReadPin(XUN_PORT, XUN_JI0_PIN);
}

static uint8_t Xunji1_ReadPin(void)
{
    return BSP_GPIO_ReadPin(XUN_PORT, XUN_JI1_PIN);
}

static uint8_t Xunji2_ReadPin(void)
{
    return BSP_GPIO_ReadPin(XUN_PORT, XUN_JI2_PIN);
}

static uint8_t Xunji3_ReadPin(void)
{
    return BSP_GPIO_ReadPin(XUN_PORT, XUN_JI3_PIN);
}

static uint8_t Xunji4_ReadPin(void)
{
    return BSP_GPIO_ReadPin(XUN_PORT, XUN_JI4_PIN);
}

void Xunji_Int_Init(void)
{
    // GPIO 初始化已由图形化配置完成
    const Xunji_ReadPin_FP pins[XUNJI_COUNT] = {
        Xunji0_ReadPin,
        Xunji1_ReadPin,
        Xunji2_ReadPin,
        Xunji3_ReadPin,
        Xunji4_ReadPin,
    };

    // 绑定读取回调到驱动层
    (void)Xunji_Driver_Bind(pins, XUNJI_COUNT);
}

//使用方式
//第一步 再sysconfig初始化gpio
//第二步 使用Xunji_Init()初始化
//第三步 在循环中使用Xunji_Get()读取巡迹状态
//黑1白0 xunji[0]~xunji[4]方向为从左到右
