
#ifndef MAIN_H_
#define MAIN_H_

//Standard Library Inclusions
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Module Inclusions
#include "GPIO_drv.h"

// Memory Size Converters
#define KB_to_B(kbs)		(1024 * kbs)
#define MB_to_B(mbs)		(1024 * KB_to_B(mbs))


#endif /* MAIN_H_ */
