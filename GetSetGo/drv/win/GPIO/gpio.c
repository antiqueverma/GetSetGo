#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "gpio.h"

static GPIO_TypeDef *GPIO_GetPort(gpio_pin_t pin)
{
    switch ((pin >> 4) & 0x0F)
    {
        case 0: return GPIOA;
        case 1: return GPIOB;
        case 2: return GPIOC;
        case 3: return GPIOD;
        case 4: return GPIOE;
        case 5: return GPIOF;
        case 6: return GPIOG;
        case 7: return GPIOH;
        case 8: return GPIOI;
        default: return NULL;
    }
}

static uint16_t GPIO_GetPinMask(gpio_pin_t pin)
{
    uint8_t pinNumber = (uint8_t)(pin & 0x0F);
    if (pinNumber >= 16U)
    {
        return 0U;
    }
    return (uint16_t)(1U << pinNumber);
}

gsg_result_t GPIO_writePin(gpio_pin_t pin, gpio_pin_state_t state)
{
    GPIO_TypeDef *port = GPIO_GetPort(pin);
    uint16_t pinMask = GPIO_GetPinMask(pin);
    if (port == NULL || pinMask == 0U)
        return GSG_INVALID_ARG;

    HAL_GPIO_WritePin(port, pinMask, state);
    return GSG_SUCCESS;
}

gsg_result_t GPIO_readPin(gpio_pin_t pin, gpio_pin_state_t *state)
{
    GPIO_TypeDef *port = GPIO_GetPort(pin);
    uint16_t pinMask = GPIO_GetPinMask(pin);
    if (state == NULL || port == NULL || pinMask == 0U)
        return GSG_INVALID_ARG;

    *state = HAL_GPIO_ReadPin(port, pinMask);
    return GSG_SUCCESS;
}
