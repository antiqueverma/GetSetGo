#ifndef GSG_BASE_H
#define GSG_BASE_H

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

// Add more module includes as needed...

#endif // GSG_BASE_H
