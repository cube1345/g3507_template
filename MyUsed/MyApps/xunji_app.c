#include "xunji_app.h"
#include "car_app.h"
#include "stdint.h"
#include "usart_app.h"

#define SPEED 200

//检测赛道
uint8_t Xunji_Check_Road(uint8_t* xunji)
{
    if (        xunji[0] ==1 ||
                xunji[1] ==1 ||
                xunji[2] ==1 ||
                xunji[3] ==1 ||
                xunji[4] ==1)
        return 1;
    else
        return 0;
}

//检测不到赛道
uint8_t Xunji_No_Road(uint8_t* xunji)
{
    return !Xunji_Check_Road(xunji);
}

//检查直角弯
uint8_t Check_Right_Angle_Turn(uint8_t *xunji)
{
    if (xunji[3] == 1 && xunji[4] == 1)
        return 1;
    return 0;
}

int8_t last_error;

//曲线循迹
int8_t  Xunji_Arc(uint8_t* xunji)
{
    int8_t error = 0;

    Xunji_Get(xunji, XUNJI_COUNT);

    uint8_t line =
        (xunji[0] << 4) |
        (xunji[1] << 3) |
        (xunji[2] << 2) |
        (xunji[3] << 1) |
        (xunji[4] << 0);

    switch(line)
    {
        //直行
        case 0b00100: error = 0; break;
        case 0b01110: error = 0; break;
            //左转
        case 0b01100: error = -1; break;
        case 0b01000: error = -2; break;
        case 0b11000: error = -3; break;
        case 0b10000: error = -4; break;
            //右转
        case 0b00010: error = 1; break;
        case 0b00110: error = 2; break;
        case 0b00011: error = 3; break;
        case 0b00001: error = 4; break;
            //丢线
        case 0b00000: error = last_error; break;
        default: error = last_error; break;
    }
    last_error = error;

    switch (error)
    {
        //左转
        case -1:Car_Run(20,0.5);break;
        case -2:Car_Run(20,0.4);break;
        case -3:Car_Run(20,0.4);break;
        case -4:Car_Run(20,0.3);break;

            //直行
        case 0:Car_Run(20,1);break;

            //右转
        case 1:Car_Run(20,1.5);break;
        case 2:Car_Run(20,1.6);break;
        case 3:Car_Run(20,1.6);break;
        case 4:Car_Run(20,1.7);break;

        default:Car_Run(20,1);break;
    }

    return error;
}

//直线循迹
int8_t Xunji_Straight(uint8_t* xunji)
{
    int8_t error = 0;

    Xunji_Get(xunji, XUNJI_COUNT);

    uint8_t line =
        (xunji[0] << 4) |
        (xunji[1] << 3) |
        (xunji[2] << 2) |
        (xunji[3] << 1) |
        (xunji[4] << 0);

    switch(line)
    {
        //直行
        case 0b00100: error = 0; break;
        case 0b01110: error = 0; break;
            //左转
        case 0b01100: error = -1; break;
        case 0b01000: error = -2; break;
        case 0b11000: error = -3; break;
        case 0b10000: error = -4; break;
            //右转
        case 0b00010: error = 1; break;
        case 0b00110: error = 2; break;
        case 0b00011: error = 3; break;
        case 0b00001: error = 4; break;
            //丢线
        case 0b00000: error = last_error; break;
        default: error = last_error; break;
    }
    last_error = error;

    switch (error)
    {
        //左转
        case -1:Car_Run(20,0.5);break;
        case -2:Car_Run(20,0.4);break;
        case -3:Car_Run(20,0.4);break;
        case -4:Car_Run(20,0.3);break;

            //直行
        case 0:Car_Run(20,1);break;

            //右转
        case 1:Car_Run(20,1.5);break;
        case 2:Car_Run(20,1.6);break;
        case 3:Car_Run(20,1.6);break;
        case 4:Car_Run(20,1.7);break;

        default:Car_Run(20,1);break;
    }

    return error;

}

//摄像头检测到赛道
uint8_t Camera_Check_Road(void)
{
    /* 读取一次“检测到赛道”事件，读完后自动清除锁存标志。 */
    return USART_App_ConsumeTrackFound();
}

//摄像头检测到离开赛道
uint8_t Camera_No_Road(void)
{
    /* 读取一次“脱离赛道”事件，读完后自动清除锁存标志。 */
    return USART_App_ConsumeTrackLost();
}

//摄像头检测到直角
uint8_t Camera_Check_Right_Angle_Turn(void)
{
    /* 读取一次“检测到直角”事件，读完后自动清除锁存标志。 */
    return USART_App_ConsumeRightAngle();
}

void Camera_Straight(void)
{
    static float integral = 0.0f;
    static float last_camera_error = 0.0f;
    const float kp = 0.010f;
    const float ki = 0.0001f;
    const float kd = 0.004f;
    const float pixel_weight = 1.2f;
    const float angle_weight = 1.0f;
    const float integral_limit = 120.0f;
    const int speed = 20;
    float pixel_error;
    float angle_error;
    float error;
    float derivative;
    float pid_output;
    float bias;

    if (!USART_App_IsStraightTrack())
    {
        integral = 0.0f;
        last_camera_error = 0.0f;
        Car_Run(speed, 1.0f);
        return;
    }

    pixel_error = (float)USART_App_GetPixelError();
    angle_error = (float)USART_App_GetAngleError();

    /* 直线循迹更关注赛道中心位置，因此像素偏差权重略高 */
    error = pixel_weight * pixel_error + angle_weight * angle_error;

    integral += error;
    if (integral > integral_limit)
    {
        integral = integral_limit;
    }
    else if (integral < -integral_limit)
    {
        integral = -integral_limit;
    }

    derivative = error - last_camera_error;
    last_camera_error = error;

    pid_output = kp * error + ki * integral + kd * derivative;

    /* bias = 1 为直行，小于 1 左转，大于 1 右转 */
    bias = 1.0f + pid_output;

    if (bias > 2.0f)
    {
        bias = 2.0f;
    }
    else if (bias < 0.0f)
    {
        bias = 0.0f;
    }

    Car_Run(speed, bias);
}

void Camera_Arc(void)
{
    static float integral = 0.0f;
    static float last_camera_error = 0.0f;
    const float kp = 0.012f;
    const float ki = 0.0002f;
    const float kd = 0.006f;
    const float pixel_weight = 1.0f;
    const float angle_weight = 1.6f;
    const float integral_limit = 120.0f;
    const int speed = SPEED;
    float pixel_error;
    float angle_error;
    float error;
    float derivative;
    float pid_output;
    float bias;

    if (!USART_App_IsArcTrack())
    {
        integral = 0.0f;
        last_camera_error = 0.0f;
        Car_Run(speed, 1.0f);
        return;
    }

    pixel_error = (float)USART_App_GetPixelError();
    angle_error = (float)USART_App_GetAngleError();

    /* 角度误差决定车头朝向，权重略高于像素偏差 */
    error = pixel_weight * pixel_error + angle_weight * angle_error;

    integral += error;
    if (integral > integral_limit)
    {
        integral = integral_limit;
    }
    else if (integral < -integral_limit)
    {
        integral = -integral_limit;
    }

    derivative = error - last_camera_error;
    last_camera_error = error;

    pid_output = kp * error + ki * integral + kd * derivative;

    /* bias = 1 为直行，小于 1 左转，大于 1 右转 */
    bias = 1.0f + pid_output;

    if (bias > 2.0f)
    {
        bias = 2.0f;
    }
    else if (bias < 0.0f)
    {
        bias = 0.0f;
    }

    Car_Run(speed, bias);
}



