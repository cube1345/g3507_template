#ifndef PLATFORM_GPIO_H
#define PLATFORM_GPIO_H

#include "stdint.h"
#include "bsp_gpio.h"

typedef GPIO_Regs *platform_gpio_port_t;
typedef uint32_t platform_gpio_pin_t;

// 平台GPIO抽象结构体，包含端口和引脚信息
typedef struct {
    platform_gpio_port_t port;
    platform_gpio_pin_t pin;
} platform_gpio_t;

void Platform_GPIO_Write(const platform_gpio_t *gpio, uint8_t value);
void Platform_GPIO_Toggle(const platform_gpio_t *gpio);
uint8_t Platform_GPIO_Read(const platform_gpio_t *gpio);
void Platform_GPIO_Set(const platform_gpio_t *gpio);
void Platform_GPIO_Clear(const platform_gpio_t *gpio);

#endif
