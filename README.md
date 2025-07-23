# GetSetGo

**GetSetGo** is a modular and portable C-based framework for embedded development.  
It provides a structured collection of libraries and drivers, designed to be easily integrable across different MCUs, IDEs, and toolchains — including FreeRTOS-based applications.

---

## 🎯 Key Features

- 🧩 Modular architecture with portable, reusable components
- 🔌 Compatibility with various MCUs (AVR, STM32, MSP430, etc.)
- 🧠 FreeRTOS-aware middleware modules
- 🧰 Utility libraries for embedded convenience
- 📡 Support for serial protocols, device drivers, and communication stacks
- 🌱 Open-source, extensible, and IDE-friendly layout

---

## 📁 Major Components

- **Include**: Header files for all portable modules and MCU-specific drivers
- **Connectivity**: Libraries for WiFi, Bluetooth, RF, RS232, RS485, and more
- **Devices**: Drivers for LCDs, Motors, Sensors, RTCs, etc.
- **Drivers (`drv`)**: Low-level MCU-specific drivers for GPIO, UART, SPI, ADC, etc.
- **HAL**: Hardware Abstraction Layer for Serial, simulation, and general peripherals
- **Middleware**: Task-based FreeRTOS modules like debug logger, modbus, and streamers
- **System**: Core startup code, fault handling, and RTOS queue utilities
- **Utilities**: Lightweight, reusable libraries like CRC, delay, bit operations, etc.
- **Docs**: Architecture notes, diagrams, and usage guides (in progress)

---

## 🔧 Getting Started

1. Add `include/` to your compiler or IDE's include path
2. Pick modules you need from `src/`, and include corresponding headers
3. Define your target MCU (e.g., `MCU_AVR`, `MCU_STM32`) for conditional headers
4. Link FreeRTOS if using middleware modules

---

## 🚀 Status

- Core structure ready and portable
- AVR and STM32 support in progress
- Middleware modules (e.g. debug, modbus) stable with FreeRTOS
- New drivers and devices being added continuously

---

## 📜 License

Planned to be released under MIT or Apache 2.0 license.  
Contribution guidelines and issue tracker coming soon.

---

## 🙌 Contribute

This project is open to contributions — feel free to fork, improve, or expand with your own device and driver support.  
Together, let’s make embedded development cleaner and faster.

---

## 🧩 Contribution Rules
To ensure consistency, portability, and maintainability across the GetSetGo (GSG) framework, all modules must strictly follow the rules below:

1. Include Guards
All header files must use include guards with the format:
#ifndef GSG_<MODULE_NAME>_H_
to prevent multiple inclusion.

2. Under Development Warning
Every new module must include the following line at the top of its .h file until it's production-ready:
#error "This module is under development and not ready for use."

3. Centralized Inclusion
Module headers must only include:
#include "gsg_base.h"
All other library includes must be centralized inside gsg_base.h.

4. Standard Library Usage
Standard C libraries (like stdio.h, stdlib.h) are allowed, but must not be included directly in modules.
They must be conditionally and centrally included through gsg_base.h.

5. Dependency Checks
Modules must generate compile-time #errors in their .h file if a required dependent module is missing or not enabled via macros.

6. Naming Convention
Functions and variables must use snake_case.
Functions must be prefixed with the module name.
e.g., modbus_init(), debug_log()

7. Macros and constants must use GSG_ prefix.
e.g., GSG_DEBUG_ENABLE, GSG_UART_TIMEOUT_MS

8. File Structure
Each module must have a .h (public API) and .c (implementation).
Internal-only symbols must be static or scoped within .c.

9. Configuration Handling
Use gsg_config.h or a dedicated <module>_config.h for all configurable options.
Use preprocessor switches to enable/disable features cleanly.

10. Memory Usage
Prefer static memory allocation.
Avoid dynamic allocation unless using a dedicated malloclite module.

11. Documentatio
Every function must have a brief description above it using /// or /** */.
Each .h and .c file must begin with a short description of the module.

12. No Circular Dependencies
Modules must be independently usable. Avoid cross-inclusions or dependency loops.

13. Testing & Versioning
Each module must have a version string in its .c file.

14. A corresponding test/demo file must be added under test/ or examples/.

---