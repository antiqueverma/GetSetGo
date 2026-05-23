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

#include "port/stm32f407ve/port.h"

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

//#if (GSG_USE_HAL_BINDING == GSG_ENABLE)
//	#if (GSG_MCU_FAMILY == GSG_MCU_STM32)
//
//	#endif
//#endif

#if GSG_USE_24CXX
  #include "device/24cXX/eeprom.h"
  #include "drv/stm32/I2C/i2c.h"
#endif

// Conditionally Include Enabled Modules
#if GSG_USE_GPIO
  #include "gpio.h"
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

//#if (GSG_USE_HAL_BINDING == GSG_ENABLE)
//	#if (GSG_MCU_FAMILY == GSG_MCU_STM32)
//  	  #include "halbindstm32.h"
//	#endif
//#endif

// Add more module includes as needed...

#endif // GSG_BASE_H_
