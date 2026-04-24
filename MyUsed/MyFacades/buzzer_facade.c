#include "buzzer_facade.h"
#include "buzzer_board.h"
#include "buzzer_driver.h"

// ================== 应用层 ================== //
/*
 * @name: Buzzer_Start/Stop
 * @note: 蜂鸣器的开关接口，直接调用Buzzer_On/Off并传入Buzzer实例
 * @return {void}
 * @author cube
 */
void Buzzer_Start(void)
{
    Buzzer_On(Buzzer_Board_GetDevice());
}

void Buzzer_Stop(void)
{
    Buzzer_Off(Buzzer_Board_GetDevice());
}

void Buzzer_Init(void)
{
    Buzzer_Board_Init();
}
