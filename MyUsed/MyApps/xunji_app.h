# ifndef __XUNJI_APP_H
# define __XUNJI_APP_H

#include "xunji_driver.h"

//检测赛道
uint8_t Xunji_Check_Road(uint8_t* xunji);

//检测不到赛道
uint8_t Xunji_No_Road(uint8_t* xunji);

//检查直角弯
uint8_t Check_Right_Angle_Turn(uint8_t *xunji);

//曲线循迹
int8_t Xunji_Arc(uint8_t* xunji);

//直线循迹
int8_t Xunji_Straight(uint8_t* xunji);



uint8_t Camera_Check_Road(void);
uint8_t Camera_No_Road(void);
uint8_t Camera_Check_Right_Angle_Turn(void);
void Camera_Straight(void);
void Camera_Arc(void);




# endif
