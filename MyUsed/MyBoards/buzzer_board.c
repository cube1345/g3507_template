#include "buzzer_board.h"
#include "ti_msp_dl_config.h"

static buzzer_t g_buzzer;

buzzer_t *Buzzer_Board_GetDevice(void)
{
    return &g_buzzer;
}

/*
 * @name: Buzzer_Board_Init
 * @note: 初始化板级蜂鸣器对象，填充GPIO和极性配置，并调用Buzzer_InitDevice完成设备初始化
 * @return {void}
 * @author cube
 */
void Buzzer_Board_Init(void)
{
    const buzzer_cfg_t buzzer_cfg = {
        .hw = {
            .gpio = {
                .port = BUZZER_PORT,
                .pin = BUZZER_BUZ_PIN,
            },
        },
        .active_high = 1U,
        .default_on = 0U,
    };

    Buzzer_InitDevice(&g_buzzer, &buzzer_cfg);
}
