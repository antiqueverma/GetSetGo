#ifndef GSG_BASE_H_
#define GSG_BASE_H_

// 📏 GSG-Wide Constants and Macros
#include "gsg_defs.h"

// 🔧 User Configuration Settings
#include "gsg_config.h"

//#ifndef GSG_OS_USED
//  #define GSG_OS_USED GSG_OS_BARE_METAL
//  #warning "GSG_OS_USED not defined, defaulting to GSG_OS_BARE_METAL"
//#endif

// freeRTOS kernel includes
#if (GSG_OS_USED == GSG_OS_FREERTOS)
  #include "FreeRTOS.h"
  #include "task.h"
  #include "semphr.h"
  #include "queue.h"
  #include "timers.h"
#include "event_groups.h"
#elif (GSG_OS_USED == GSG_OS_CMSIS_V2)
  #error "NO CMSIS V2 SUPPORT"
#endif

#if GSG_USE_SYS == GSG_ENABLE
  #include "sys/sys.h"
#endif

#if GSG_USE_24CXX
  #include "device/24cXX/eeprom.h"
  #ifndef GSG_USE_I2C
    #define GSG_USE_I2C
  #endif
#endif

#if GSG_USE_SERIAL == GSG_ENABLE
  #include "services/serial/serial.h"
#endif

#if GSG_USE_DEBUG == GSG_ENABLE
  #include "services/debug/debug.h"
#endif

#if GSG_USE_STREAM == GSG_ENABLE
  #include "services/stream/stream.h"
#endif

#if GSG_USE_MODBUS == GSG_ENABLE
  #include "connectivity/modbus/modbus.h"
#endif

#if GSG_USE_QUEUE == GSG_ENABLE
  #include "queue.h"
#endif

#if (GSG_USE_NVM == GSG_ENABLE)
  #include "sys/nvm/nvm.h"
#endif

#if (GSG_USE_W25QXX == GSG_ENABLE)
  #include "device/w25qXX/flash.h"
#endif

#if (GSG_USE_WDT == GSG_ENABLE)
  #include "wdt.h"
#endif

#if (GSG_USE_TIME == GSG_ENABLE)
  #include "time.h"
#endif

#if (GSG_USE_WAVEGEN == GSG_ENABLE)
  #include "wavegen.h"
#endif

#if (GSG_USE_CSV == GSG_ENABLE)
  #include "csv.h"
#endif

#if (GSG_USE_DSP == GSG_ENABLE)
  #include "dsp.h"
#endif

#if (GSG_USE_SVAR == GSG_ENABLE)
  #include "services/svar/svar.h"
#endif

#if (GSG_USE_TASKMGR == GSG_ENABLE)
  #include "services/taskman/taskmgr.h"
#endif

#if (GSG_USE_EVENT == GSG_ENABLE)
  #include "event.h"
#endif

#if (GSG_USE_FSLITE == GSG_ENABLE)
  #include "fslite.h"
#endif

#if (GSG_USE_IO == GSG_ENABLE)
  #include "io.h"
#endif

#if (GSG_USE_MEMALLOC == GSG_ENABLE)
  #include "memalloc.h"
#endif

#if (GSG_USE_ETHERNET == GSG_ENABLE)
  #include "ethernet.h"
#endif

#if (GSG_USE_WIFI == GSG_ENABLE)
  #include "wifi.h"
#endif

#if (GSG_USE_BLUETOOTH == GSG_ENABLE)
  #include "bluetooth.h"
#endif

#if (GSG_USE_RF433 == GSG_ENABLE)
  #include "rf433.h"
#endif

#if (GSG_USE_SYSINFO == GSG_ENABLE)
  #include "utilities/sysinfo.h"
#endif

#if (GSG_USE_ESP8266 == GSG_ENABLE)
  #include "connectivity/esp8266/esp8266.h"
#endif

// Add more module includes as needed...


/* Driver Dependencies */ 
#if defined(GSG_PLATFORM_STM32)

  #if GSG_USE_I2C == GSG_ENABLE
    #include "drv/stm32/I2C/i2c.h"
  #endif

  #if GSG_USE_GPIO
    #include "drv/stm32/GPIO/gpio.h"
  #endif

  #if GSG_USE_UART == GSG_ENABLE
    #include "drv/stm32/UART/uart.h"
  #endif

  #if GSG_USE_SPI == GSG_ENABLE
    #include "drv/stm32/SPI/spi.h"
  #endif
  
  #include "port/stm32f407ve/port.h"
  
#elif defined(GSG_PLATFORM_WINDOWS)
  // Include Windows-specific headers or definitions if needed
  #include "port/windows/port.h"
  
  #if GSG_USE_SPI == GSG_ENABLE
    #include "drv/win/SPI/spi.h"
  #endif

  #warning "Windows platform selected - ensure appropriate drivers and modules are implemented"
#endif


#endif // GSG_BASE_H_
