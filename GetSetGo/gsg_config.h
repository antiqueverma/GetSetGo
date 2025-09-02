#ifndef GSG_CONFIG_H_
#define GSG_CONFIG_H_

#include "gsg_defs.h"  // always include master defines

// Set your project's setting:
#define GSG_MCU_FAMILY		GSG_MCU_STM32
#define GSG_OS_USED         GSG_OS_FREERTOS

/* Enable/disable modules */ 
#define GSG_USE_MEMALLOC    GSG_DISABLE

// Drivers
#define GSG_USE_GPIO		GSG_DISABLE

// Connectivity
#define GSG_USE_BLUETOOTH   GSG_DISABLE
#define GSG_USE_ETHERNET    GSG_DISABLE
#define GSG_USE_RF433       GSG_DISABLE
#define GSG_USE_RS232       GSG_DISABLE
#define GSG_USE_RS485       GSG_DISABLE
#define GSG_USE_MODBUS		GSG_ENABLE
#define GSG_USE_WIFI        GSG_DISABLE
#define GSG_USE_ESP8266     GSG_DISABLE

// Devices
#define GSG_USE_LCD1602     GSG_DISABLE
#define GSG_USE_LCD12864    GSG_DISABLE
#define GSG_USE_RTC         GSG_DISABLE

// Services
#define GSG_USE_DEBUG		GSG_ENABLE
#define GSG_USE_EVENT       GSG_DISABLE
#define GSG_USE_FSLITE      GSG_DISABLE
#define GSG_USE_STREAM		GSG_DISABLE
#define GSG_USE_IO          GSG_DISABLE
#define GSG_USE_NVM         GSG_DISABLE
#define GSG_USE_SERIAL      GSG_ENABLE
#define GSG_USE_STREAM      GSG_DISABLE
#define GSG_USE_WDT         GSG_DISABLE

// Utilities 
#define GSG_USE_SYSINFO     GSG_ENABLE
#define GSG_USE_QUEUE		GSG_DISABLE
#define GSG_USE_CSV         GSG_DISABLE
#define GSG_USE_DSP         GSG_DISABLE
#define GSG_USE_SVAR        GSG_DISABLE
#define GSG_USE_TIME        GSG_DISABLE
#define GSG_USE_WAVEGEN     GSG_DISABLE  

#define GSG_USE_HAL_BINDING GSG_ENABLE

#if (GSG_USE_HAL_BINDING == GSG_ENABLE)
    // #define HAL_USE_UART0
     #define HAL_USE_UART1
    // #define HAL_USE_UART2
    #define HAL_USE_UART3
    // #define HAL_USE_UART4
    #define HAL_USE_UART5
    // #define HAL_USE_I2C0
    // #define HAL_USE_I2C1
    // #define HAL_USE_I2C2
    // #define HAL_USE_I2C3
    // #define HAL_USE_I2C4
    // #define HAL_USE_SPI0
    // #define HAL_USE_SPI1
    // #define HAL_USE_SPI2
    // #define HAL_USE_SPI3
    // #define HAL_USE_SPI4
#endif

// Optional: Other configurable values
#define GSG_LOG_LEVEL     GSG_DEFAULT_LOG_LEVEL

#endif // GSG_CONFIG_H_
