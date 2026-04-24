#include "car_app.h"

extern int g_target_speedA;
extern int g_target_speedB;

void Car_Run(int speed, float bias)
{
    g_target_speedA = speed*bias;
    g_target_speedB = speed*(2-bias);
}

