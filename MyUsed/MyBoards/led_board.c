#include "led_board.h"
#include "ti_msp_dl_config.h"

static led_t g_blue_led;
static led_t g_red_led;

led_t *LED_Board_GetBlue(void)
{
    return &g_blue_led;
}

led_t *LED_Board_GetRed(void)
{
    return &g_red_led;
}

void LED_Board_Init(void)
{
    const led_cfg_t blue_cfg = {
        .hw = {
            .gpio = {
                .port = LED_BLUE_PORT,
                .pin = LED_BLUE_PIN,
            },
        },
        .active_low = 1U,
        .default_on = 0U,
    };
    const led_cfg_t red_cfg = {
        .hw = {
            .gpio = {
                .port = LED_RED_PORT,
                .pin = LED_RED_PIN,
            },
        },
        .active_low = 1U,
        .default_on = 0U,
    };

    LED_Init(&g_blue_led, &blue_cfg);
    LED_Init(&g_red_led, &red_cfg);
}
