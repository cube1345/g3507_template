#ifndef ENCODER_FACADE_H
#define ENCODER_FACADE_H

#include "stdint.h"
#include "../MyDevices/encoder_driver.h"

extern int Get_Encoder_countA;
extern int Get_Encoder_countB;

void Encoder_Init(void);
int32_t Encoder_GetCountA(void);
int32_t Encoder_GetCountB(void);
void Encoder_ResetCountA(void);
void Encoder_ResetCountB(void);
void Encoder_SyncLegacyCounts(void);
void Encoder_ResetLegacyCounts(void);

#endif
