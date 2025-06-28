#ifndef GSG_DEFS_H
#define GSG_DEFS_H

// Module toggle macros
#define GSG_ENABLE     1
#define GSG_DISABLE    0

#define GSG_MCU_AVR			1
#define GSG_MCU_STM32		2
#define GSG_MCU_MSP430		3
#define GSG_MCU_PIC8		4
#define GSG_MCU_PIC16		5
#define GSG_MCU_PIC32		6
#define GSG_MCU_PICESP32	7

#define GSG_OS_BARE_METAL   0
#define GSG_OS_FREERTOS     1
#define GSG_OS_ZEPHYR       2


// Module ID macros (optional use)
#define GSG_MODULE_DEBUG     1
#define GSG_MODULE_STREAM    2
#define GSG_MODULE_MODBUS    3

// Other framework-wide constants
#define GSG_DEFAULT_LOG_LEVEL 2

#endif // GSG_DEFS_H
