

#include "gsg_config.h"
#ifndef GSG_PORT_H_
#define GSG_PORT_H_
#include "stm32f1xx_hal.h"

// Port definitions for the stm32f103c8 mcu
typedef enum {
    SYS_PER_RCC,
    /* UART/USART Peripherals */
    SYS_PER_UART1,
    SYS_PER_UART2,
    SYS_PER_UART3,
    /* GPIO Ports */
    SYS_PER_GPIOA,
    SYS_PER_GPIOB,
    SYS_PER_GPIOC,
    SYS_PER_GPIOD,
    /* SPI Peripherals */
    SYS_PER_SPI1,
    SYS_PER_SPI2,
    /* I2C Peripherals */
    SYS_PER_I2C1,
    SYS_PER_I2C2,
    /* Timer Peripherals */
    SYS_PER_TIM1,
    SYS_PER_TIM2,
    SYS_PER_TIM3,
    SYS_PER_TIM4,
    /* ADC Peripherals */
    SYS_PER_ADC1,
    SYS_PER_ADC2,
    /* CAN Peripherals */
    SYS_PER_CAN1,
    /* DAC Peripheral */
    SYS_PER_DAC,
    /* USB and Ethernet */
    SYS_PER_USB_OTG_FS,
    SYS_PER_USB_OTG_HS,
    /* Other Peripherals */
    SYS_PER_DCMI,
    SYS_PER_SDIO,
    SYS_PER_RTC,
    SYS_PER_IWDG,
    SYS_PER_WWDG,
    SYS_PER_PWR,
} sys_peripheral_t;

#define PORT_PERIPHERAL_UART_COUNT      3
#define PORT_PERIPHERAL_I2C_COUNT       2
#define PORT_PERIPHERAL_SPI_COUNT       2
#define PORT_PERIPHERAL_TIM_COUNT       4
#define PORT_PERIPHERAL_ADC_COUNT       2
#define PORT_PERIPHERAL_CAN_COUNT       1

#define PORT_PERIPHERAL_ADC_CHANNEL_COUNT     12
#define PORT_PERIPHERAL_GPI_CHANNEL_COUNT     48
#define PORT_PERIPHERAL_GPO_CHANNEL_COUNT     48

// extern SPI_HandleTypeDef hspi1;
// extern SPI_HandleTypeDef hspi2;

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;

extern CAN_HandleTypeDef hcan;

#endif /* GSG_PORT_H_*/
