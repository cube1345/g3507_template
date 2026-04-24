#ifndef LED_INT_H
#define LED_INT_H

#include "led_board.h"

static inline void LED_Int_Init(void)
{
    LED_Board_Init();
}

static inline led_t *LED_Int_GetBlue(void)
{
    return LED_Board_GetBlue();
}

static inline led_t *LED_Int_GetRed(void)
{
    return LED_Board_GetRed();
}

#endif
