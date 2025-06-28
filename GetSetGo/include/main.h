
#ifndef MAIN_H_
#define MAIN_H_

//Standard Library Inclusions
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

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
#define BIT(n)              (1U << (n))
#define SET_BIT(x, n)       ((x) |= BIT(n))
#define CLR_BIT(x, n)       ((x) &= ~BIT(n))
#define TOG_BIT(x, n)       ((x) ^= BIT(n))
#define GET_BIT(x, n)       (((x) >> (n)) & 1U)

// Loop Helpers
#define FOREVER             for(;;)
#define LOOP(i, n)          for (int i = 0; i < (n); ++i)

// Math Utilities
#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))
#define CLAMP(x, lo, hi)    (MAX((lo), MIN((x), (hi))))
#define ABS(x)              (((x) < 0) ? -(x) : (x))
#define SIGN(x)             (((x) > 0) - ((x) < 0))

// GSG Module Inclusions
#include "gsg_config.h"

#endif /* MAIN_H_ */
