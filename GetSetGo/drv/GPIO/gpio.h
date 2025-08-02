
#ifndef GPIO_H_
#define GPIO_H_

#include "gsg_base.h"

#if (GSG_MCU_FAMILY == GSG_MCU_AVR)
    #include "AVR/GPIO_drv.h"
#elif (GSG_MCU_FAMILY == GSG_MCU_ESP32)

#endif

#include "gpio_pin_defs.h"

#endif  // GPIO_H_