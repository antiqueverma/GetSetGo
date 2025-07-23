#ifndef GSG_BASE_H_
#define GSG_BASE_H_

// 🔧 User Configuration Settings
#include "gsg_config.h"

// 📏 GSG-Wide Constants and Macros
#include "gsg_defs.h"

// 📦 Conditionally Include Enabled Modules
#if GSG_USE_GPIO
  #include "gpio.h"
#endif

#if GSG_USE_DEBUG == GSG_ENABLE
  #include "debug.h"
#endif

#if GSG_USE_STREAM == GSG_ENABLE
  #include "stream.h"
#endif

#if GSG_USE_MODBUS == GSG_ENABLE
  #include "modbus.h"
#endif

#if GSG_USE_QUEUE == GSG_ENABLE
  #include "queue.h"
#endif

#if GSG_USE_NVM == GSG_ENABLE
  #include "nvm.h"
#endif

#if GSG_USE_SERIAL == GSG_ENABLE
  #include "serial.h"
#endif

#if GSG_USE_WDT == GSG_ENABLE
  #include "wdt.h"
#endif

#if GSG_USE_TIME == GSG_ENABLE
  #include "time.h"
#endif

#if GSG_USE_WAVEGEN == GSG_ENABLE
  #include "wavegen.h"
#endif

#if GSG_USE_CSV == GSG_ENABLE
  #include "csv.h"
#endif

#if GSG_USE_DSP == GSG_ENABLE
  #include "dsp.h"
#endif

#if GSG_USE_SVAR == GSG_ENABLE
  #include "svar.h"
#endif

#if GSG_USE_EVENT == GSG_ENABLE
  #include "event.h"
#endif

#if GSG_USE_FSLITE == GSG_ENABLE
  #include "fslite.h"
#endif

#if GSG_USE_IO == GSG_ENABLE
  #include "io.h"
#endif

#if GSG_USE_MEMALLOC == GSG_ENABLE
  #include "memalloc.h"
#endif

#if GSG_USE_ETHERNET == GSG_ENABLE
  #include "ethernet.h"
#endif

#if GSG_USE_WIFI == GSG_ENABLE
  #include "wifi.h"
#endif

#if GSG_USE_BLUETOOTH == GSG_ENABLE
  #include "bluetooth.h"
#endif

#if GSG_USE_RF433 == GSG_ENABLE
  #include "rf433.h"
#endif

#if GSG_USE_RS485 == GSG_ENABLE
  #include "rs485.h"
#endif

#if GSG_USE_

// Add more module includes as needed...

#endif // GSG_BASE_H
