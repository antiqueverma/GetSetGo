#ifndef GSG_CONFIG_H
#define GSG_CONFIG_H

#include "gsg_defs.h"  // always include master defines

// Select MCU (affects headers used)
#define GSG_MCU_FAMILY		GSG_MCU_AVR
// #define MCU_STM32

// Enable/disable modules
// Drivers
#define GSG_USE_GPIO		GSG_ENABLE

// Protocols
#define GSG_USE_MODBUS		GSG_DISABLE

// Middleware
#define GSG_USE_DEBUG		GSG_DISABLE
#define GSG_USE_STREAM		GSG_DISABLE


#define GSG_USE_QUEUE		GSG_DISABLE



// Optional: Other configurable values
#define GSG_LOG_LEVEL     GSG_DEFAULT_LOG_LEVEL


#include "gsg_base.h"

#endif // GSG_CONFIG_H