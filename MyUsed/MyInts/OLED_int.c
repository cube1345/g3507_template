#include "OLED_int.h"

//通过函数指针绑定OLED的I2C所需要的写总线函数
static void OLED_SCL_Write(uint8_t state)
{
    if (state)
    {
        DL_GPIO_setPins(OLED_PORT, OLED_SCL0_PIN);
    }
    else
    {
        DL_GPIO_clearPins(OLED_PORT, OLED_SCL0_PIN);
    }

    /*如果单片机速度过快，可在此添加适量延时，以避免超出I2C通信的最大速度*/
//    delay_cycles(3);
}

static void OLED_SDA_Write(uint8_t state)
{
    if (state)
    {
        DL_GPIO_setPins(OLED_PORT, OLED_SDA0_PIN);
    }
    else
    {
        DL_GPIO_clearPins(OLED_PORT, OLED_SDA0_PIN);
    }

    /*如果单片机速度过快，可在此添加适量延时，以避免超出I2C通信的最大速度*/
//    delay_cycles(3);
}

void OLED_Driver_Init(void)
{
    OLED_I2C_Init(OLED_SCL_Write,OLED_SDA_Write);
}

