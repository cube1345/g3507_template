#include "xunji_driver.h"
#include "xunji_int.h"

// 保存引脚读取回调
static Xunji_ReadPin_FP g_xunji_pins[XUNJI_COUNT];
// 初始化标志
static uint8_t g_xunji_inited = 0;

int Xunji_Driver_Bind(const Xunji_ReadPin_FP *pins, uint8_t count)
{
    // 基本参数检查
    if (pins == 0 || count < XUNJI_COUNT) {
        g_xunji_inited = 0;
        return -1;
    }

    for (uint8_t i = 0; i < XUNJI_COUNT; i++) {
        // 防止空函数指针导致硬Fault
        if (pins[i] == 0) {
            g_xunji_inited = 0;
            return -1;
        }
        g_xunji_pins[i] = pins[i];
    }

    g_xunji_inited = 1;
    return 0;
}

int Xunji_Get(uint8_t *xunji, uint8_t count)
{
    // 参数与初始化状态检查
    if (xunji == 0 || count < XUNJI_COUNT || g_xunji_inited == 0) {
        return -1;
    }

    for (uint8_t i = 0; i < XUNJI_COUNT; i++) {
        xunji[i] = g_xunji_pins[i]();
    }

    return 0;
}

void Xunji_Init(void)
{
    Xunji_Int_Init();
}
