#ifndef GSG_MCU_H_
#define GSG_MCU_H_

/*
 * Single place that selects the HAL + port header for the target MCU family.
 * Detection relies on the device macros defined by STM32CubeIDE/CMSIS
 * (STM32F103xB, STM32F407xx, ...), so no custom define is needed.
 */
#if defined(STM32F1xx) || defined(STM32F1XX) || defined(STM32F103xB) || defined(STM32F103x6) || \
    defined(STM32F103xE) || defined(STM32F103xG)
  #define GSG_MCU_STM32F1
  #include "stm32f1xx_hal.h"
  #include "port/stm32f103c8/port.h"
#elif defined(STM32F4xx) || defined(STM32F407VE) || defined(STM32F407xx) || defined(STM32F405xx) || \
      defined(STM32F415xx) || defined(STM32F417xx) || defined(STM32F427xx) || defined(STM32F429xx)
  #define GSG_MCU_STM32F4
  #include "stm32f4xx_hal.h"
  #include "port/stm32f407ve/port.h"
#else
  #error "gsg_mcu.h: unsupported MCU - define a supported STM32 device macro"
#endif

#endif /* GSG_MCU_H_ */
