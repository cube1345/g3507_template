#include "buzzer_board.h"
#include "ti_msp_dl_config.h"

static buzzer_t g_buzzer;

buzzer_t *Buzzer_Board_GetDevice(void)
{
    return &g_buzzer;
}

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
