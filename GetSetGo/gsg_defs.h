#ifndef GSG_DEFS_H_
#define GSG_DEFS_H_

// Memory Size Converters
#define KB_to_B(kbs)		(1024 * kbs)
#define MB_to_B(mbs)		(1024 * KB_to_B(mbs))

// Time Constants (generic tick-based systems)
#define SEC_to_MS(sec)      ((sec) * 1000UL)
#define MS_to_SEC(ms)       ((ms) / 1000UL)

// Common Constants
#define PASS        1
#define SUCCESS     1
#define FAIL        0
#define TRUE        1
#define FALSE       0
#define VALID       1
#define INVALID     0
#define ENABLED     1
#define DISABLED    0
#define SET         1
#define RESET       0
#define ON          1
#define OFF         0
#define YES         1
#define NO          0

// Character Constants
#define CHAR_CR             '\r'
#define CHAR_LF             '\n'
#define CHAR_NUL            '\0'
#define CHAR_SPACE          ' '
#define CHAR_TAB            '\t'

// Bit Manipulation Macros
#define BIT_MASK(n)         (1U << (n))
#ifndef SET_BIT
  #define SET_BIT(x, n)     ((x) |= BIT_MASK(n))
#endif
#ifndef RESET_BIT
  #define RESET_BIT(x, n)   ((x) &= ~BIT_MASK(n))
#endif

#define TOG_BIT(x, n)       ((x) ^= BIT_MASK(n))
#define GET_BIT(x, n)       (((x) >> (n)) & 1U)

// Loop Helpers
#define FOREVER             for(;;)
#define LOOP(i, n)          for (int i = 0; i < (n); ++i)

// Math Utilities
#ifndef MIN
	#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#endif // MIN
#ifndef MAX
	#define MAX(a, b)           ((a) > (b) ? (a) : (b))
#endif // MAX

#define CLAMP(x, lo, hi)    (MAX((lo), MIN((x), (hi))))
#define ABS(x)              (((x) < 0) ? -(x) : (x))
#define SIGN(x)             (((x) > 0) - ((x) < 0))

// Module toggle macros
#define GSG_ENABLE     1
#define GSG_DISABLE    0

typedef enum {
	GSG_MCU_AVR, 
	GSG_MCU_STM32,		
	GSG_MCU_MSP430,		
	GSG_MCU_PIC8,		
	GSG_MCU_PIC16,		
	GSG_MCU_PIC32,		
	GSG_MCU_PICESP32,	
} gsg_mcu_fam_t;

typedef enum {
	GSG_OS_BARE_METAL,
	GSG_OS_FREERTOS,
	GSG_OS_CMSIS_V1,
	GSG_OS_CMSIS_V2,
	GSG_OS_ZEPHYR
} gsg_os_t;

// Module ID macros (optional use)
#define GSG_MODULE_DEBUG     1
#define GSG_MODULE_STREAM    2
#define GSG_MODULE_MODBUS    3

// Other framework-wide constants
#define GSG_DEFAULT_LOG_LEVEL 2

typedef enum {
    GSG_ERROR,
    GSG_INVALID_ARG = GSG_ERROR,
    GSG_NOT_FOUND = GSG_ERROR,
	GSG_NOT_IMPLEMENTED = GSG_ERROR,
	GSG_OVERFLOW = GSG_ERROR,
    GSG_OK,
	GSG_SUCCESS = GSG_OK,
    GSG_TIMEOUT,
    GSG_BUSY,
} gsg_result_t;

#endif // GSG_DEFS_H_
