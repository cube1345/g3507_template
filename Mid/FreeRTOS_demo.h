#ifndef __FREERTOS_H
#define __FREERTOS_H


#include "my_freetros_lib.h"

#include "IMU.h"
#include "led_facade.h"
#include "buzzer_facade.h"
#include "OLED_int.h"
#include "OLED_menu.h"
#include "xunji_driver.h"
#include "xunji_app.h"
#include "key_driver.h"
#include "car_app.h"
#include "my_math.h"
#include "usart_app.h"
#include "encoder_driver.h"

void FreeRTOS_Start(void);

void Mutex_Printf(const char *format, ...);
void Show_System_Stats(void);



#endif
