#ifndef __BUZZER_INT_H
#define __BUZZER_INT_H

#include "buzzer_board.h"

static inline void Buzzer_Int_Init(void)
{
    Buzzer_Board_Init();
}

static inline buzzer_t *Buzzer_Int_GetDevice(void)
{
    return Buzzer_Board_GetDevice();
}

#endif
