

#include "gsg_config.h"
#ifndef GSG_PORT_H_
#define GSG_PORT_H_

// Port definitions fr the stm32f407ve mcu

typedef enum {
    SYS_PER_RCC,
    /* UART/USART Peripherals */
    SYS_PER_UART1,
    SYS_PER_UART2,
    SYS_PER_UART3,
    SYS_PER_UART4,
    SYS_PER_UART5,
    SYS_PER_UART6,
    /* GPIO Ports */
    SYS_PER_GPIOA,
    SYS_PER_GPIOB,
    SYS_PER_GPIOC,
    SYS_PER_GPIOD,
    SYS_PER_GPIOE,
    SYS_PER_GPIOF,
    SYS_PER_GPIOG,
    SYS_PER_GPIOH,
    SYS_PER_GPIOI,
    /* SPI Peripherals */
    SYS_PER_SPI1,
    SYS_PER_SPI2,
    SYS_PER_SPI3,
    /* I2C Peripherals */
    SYS_PER_I2C1,
    SYS_PER_I2C2,
    SYS_PER_I2C3,
    /* Timer Peripherals */
    SYS_PER_TIM1,
    SYS_PER_TIM2,
    SYS_PER_TIM3,
    SYS_PER_TIM4,
    SYS_PER_TIM5,
    SYS_PER_TIM6,
    SYS_PER_TIM7,
    SYS_PER_TIM8,
    SYS_PER_TIM9,
    SYS_PER_TIM10,
    SYS_PER_TIM11,
    SYS_PER_TIM12,
    SYS_PER_TIM13,
    SYS_PER_TIM14,
    /* ADC Peripherals */
    SYS_PER_ADC1,
    SYS_PER_ADC2,
    SYS_PER_ADC3,
    /* CAN Peripherals */
    SYS_PER_CAN1,
    SYS_PER_CAN2,
    /* DAC Peripheral */
    SYS_PER_DAC,
    /* USB and Ethernet */
    SYS_PER_USB_OTG_FS,
    SYS_PER_USB_OTG_HS,
    SYS_PER_ETH,
    /* Other Peripherals */
    SYS_PER_DCMI,
    SYS_PER_SDIO,
    SYS_PER_RTC,
    SYS_PER_IWDG,
    SYS_PER_WWDG,
    SYS_PER_PWR,
} sys_peripheral_t;

#endif /* GSG_PORT_H_*/
