#ifndef LED_BOARD_H
#define LED_BOARD_H

#include "led_driver.h"

void LED_Board_Init(void);
led_t *LED_Board_GetBlue(void);
led_t *LED_Board_GetRed(void);

#endif
