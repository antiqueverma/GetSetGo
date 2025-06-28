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
