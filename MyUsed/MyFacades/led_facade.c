#include "led_facade.h"
#include "led_board.h"

// ================== 应用层 ================== //
/*
 * @name: LED_B_On/Off/Toggle
 * @note: 蓝色LED的开关接口，直接调用LED_On/Off/Toggle并传入蓝色LED实例
 * @return {void}
 * @author cube
 */
void LED_B_On(void)
{
    LED_On(LED_Board_GetBlue());
}

void LED_B_Off(void)
{
    LED_Off(LED_Board_GetBlue());
}

void LED_B_Toggle(void)
{
    LED_Toggle(LED_Board_GetBlue());
}

/*
 * @name: LED_R_On/Off/Toggle
 * @note: 红色LED的开关接口，直接调用LED_On/Off/Toggle并传入红色LED实例
 * @return {void}
 * @author cube
 */
void LED_R_On(void)
{
    LED_On(LED_Board_GetRed());
}

void LED_R_Off(void)
{
    LED_Off(LED_Board_GetRed());
}

void LED_R_Toggle(void)
{
    LED_Toggle(LED_Board_GetRed());
}

void LED_Driver_Init(void)
{
    LED_Board_Init();
}
