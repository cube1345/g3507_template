#include "platform_gpio.h"

void Platform_GPIO_Write(const platform_gpio_t *gpio, uint8_t value)
{
    if (gpio == 0) {
        return;
    }

    BSP_GPIO_WritePin(gpio->port, gpio->pin, value);
}

void Platform_GPIO_Toggle(const platform_gpio_t *gpio)
{
    if (gpio == 0) {
        return;
    }

    BSP_GPIO_TogglePin(gpio->port, gpio->pin);
}

uint8_t Platform_GPIO_Read(const platform_gpio_t *gpio)
{
    if (gpio == 0) {
        return 0U;
    }

    return BSP_GPIO_ReadPin(gpio->port, gpio->pin);
}

void Platform_GPIO_Set(const platform_gpio_t *gpio)
{
    if (gpio == 0) {
        return;
    }

    BSP_GPIO_SetPin(gpio->port, gpio->pin);
}

void Platform_GPIO_Clear(const platform_gpio_t *gpio)
{
    if (gpio == 0) {
        return;
    }

    BSP_GPIO_ClearPin(gpio->port, gpio->pin);
}
