# Get Set Go - MCU Hardware Simulator 🚀

**Get Set Go** is a modern hardware peripheral simulator built for embedded engineers porting **FreeRTOS** applications to **Windows (GCC port)**. 

When running an embedded RTOS framework on Windows, hardware peripherals (GPIO, UART, EEPROM, Flash, ADC, 16x2 LCD) are simulated by counterpart drivers that communicate with **Get Set Go** over TCP using simple, lightweight JSON packets.

---

## 🌟 Key Features

1. **Modular Desktop Menu Bar**:
   - **View**: Toggle visibility of all 6 hardware modules (**GPIO, Console, EEPROM, Flash, ADC, 16x2 Display**) with individual checkmarks and keyboard shortcuts (`Ctrl+1` .. `Ctrl+6`).
   - **Arrange**:
     - **Tile**: Responsive grid of available open module windows across the screen.
     - **Tabbed**: Full-window document tab mode; selecting a tab opens the module full screen.
   - **Connect**: Live status pill and dialog to configure TCP Host IP (`127.0.0.1`), Port (`9000`), Socket Role (TCP Server vs TCP Client), and auto-reconnect.
   - **Tools**: Live raw JSON packet stream inspector with filtering and built-in FreeRTOS mock traffic generator.

2. **Astonishing Hardware Visualizations & Interactivity**:
   - **GPIO**: Ports A and B, 16 pins with live glowing LED states (3.3V / 0V), direction badges (IN/OUT), pull-up/down resistors, clickable user stimulation push-buttons, and microsecond event logs.
   - **UART / Serial Console**: Cyberpunk terminal monitor with baud rate selector (9600 to 921600), autoscroll, command history (Up/Down arrow memory), and quick macros.
   - **EEPROM Memory Viewer**: 512B / 1KB hex dump table with decoded ASCII, live cell pulse highlights on MCU writes, and click-to-edit memory cells.
   - **NOR/On-Chip Flash**: 8 sector blocks (4KB each) showing erase cycles, flash bitwise-AND programming physics, sector erase animation, and page hex dump.
   - **ADC Sampler & Injector**: 8 channels with calibrated voltage meters (0.00V - 3.30V), 12-bit raw integer readouts, sparkline waveforms, and interactive potentiometers to inject analog voltages back into the FreeRTOS MCU!
   - **HD44780 16x2 LCD Display**: Realistic 1602 LCD module with 5x8 dot matrix font emulation, switchable backlight themes (Classic Blue, Yellow-Green, Amber, Dark OLED), hardware blinking cursor, and contrast knob simulation.

3. **Bidirectional Communication**:
   - MCU to Simulator: Drivers send status changes.
   - Simulator to MCU: User interactions (pressing a GPIO input button or sliding an ADC potentiometer) immediately send packets back over TCP to your FreeRTOS Windows process.

---

## 📁 Project Architecture & File Separation

```
winsim/
├── backend/
│   ├── main.py                     # FastAPI server + WebSocket bridge + static files
│   ├── connection_manager.py       # Dedicated TCP connection & packet routing manager
│   ├── simulate_freertos_app.py    # Demo FreeRTOS client sending realistic test packets
│   ├── freertos_driver_example.c   # Ready-to-use C Winsock template for Windows drivers
│   └── modules/                    # Separate backend software files for each module
│       ├── gpio_module.py
│       ├── console_module.py
│       ├── eeprom_module.py
│       ├── flash_module.py
│       ├── adc_module.py
│       └── display_module.py
├── frontend/
│   ├── src/
│   │   ├── App.tsx                 # Root workbench application
│   │   ├── services/
│   │   │   └── packetConnection.ts # Dedicated connection & packet handling file
│   │   ├── types/
│   │   │   └── modules.ts          # TypeScript interfaces for all modules & packets
│   │   ├── modules/                # Separate frontend software files for each module
│   │   │   ├── GpioModule.tsx
│   │   │   ├── ConsoleModule.tsx
│   │   │   ├── EepromModule.tsx
│   │   │   ├── FlashModule.tsx
│   │   │   ├── AdcModule.tsx
│   │   │   └── Display16x2Module.tsx
│   │   └── components/
│   │       ├── menu/MenuBar.tsx    # Desktop-style File menu bar
│   │       ├── modals/             # ConnectModal & PacketInspectorModal
│   │       └── layout/             # TileLayout, TabLayout, ModuleWrapper
├── start_all.bat                   # 1-Click launcher
└── test_simulation.bat             # 1-Click FreeRTOS driver simulator test
```

---

## 🚀 Quick Start

### 1. Launch the Application
Simply double-click `start_all.bat` or run:
```powershell
cd backend
python main.py
```
Then open your browser at **`http://localhost:8000`**.

### 2. Run the FreeRTOS Driver Simulator Test
In another terminal, run:
```powershell
cd backend
python simulate_freertos_app.py
```
You will instantly see:
- Heartbeat LED blinking on GPIO Pin PA5.
- Boot messages streaming into the UART Console.
- LCD 16x2 showing live system uptime and temperature.
- ADC channels displaying live analog sine waveforms.
- EEPROM memory table highlighting written log addresses.
- Flash sectors undergoing sector erase & page programming.

---

## 📡 TCP JSON Packet Protocol Specification

Packets are simple, newline-delimited (`\n`) JSON strings sent over TCP (port `9000`).

### 1. GPIO Packets
```json
// Write pin state (from MCU)
{"module": "gpio", "action": "write", "port": "A", "pin": 5, "state": 1}

// Configure direction (from MCU)
{"module": "gpio", "action": "config_direction", "port": "A", "pin": 0, "direction": "in", "label": "USER_BTN"}

// Input changed by user in UI (sent to MCU)
{"module": "gpio", "action": "input_change", "port": "A", "pin": 0, "state": 1}
```

### 2. UART Console Packets
```json
// Transmit string from MCU
{"module": "console", "action": "tx", "text": "Task initialized.\r\n"}

// User typed string in UI (sent to MCU)
{"module": "console", "action": "rx", "text": "help\r\n"}
```

### 3. ADC Packets
```json
// ADC sample update (from MCU)
{"module": "adc", "action": "update", "channel": 0, "voltage": 3.15, "raw": 3910}

// User potentiometer moved in UI (sent to MCU)
{"module": "adc", "action": "input_change", "channel": 0, "voltage": 2.50, "raw": 3103}
```

### 4. 16x2 LCD Display Packets
```json
// Write text to line and column
{"module": "display", "action": "write", "line": 0, "col": 0, "text": "Hello FreeRTOS", "backlight": true}

// Clear screen
{"module": "display", "action": "clear"}
```

### 5. EEPROM Packets
```json
// Write bytes to memory address
{"module": "eeprom", "action": "write", "address": 16, "data": [0xDE, 0xAD, 0xBE, 0xEF]}

// Fill entire memory
{"module": "eeprom", "action": "fill", "value": 255}
```

### 6. Flash Packets
```json
// Erase 4KB sector
{"module": "flash", "action": "sector_erase", "sector": 2}

// Program page bytes
{"module": "flash", "action": "page_program", "sector": 2, "address": 8192, "data": [1, 2, 3, 4]}
```

---

## 🛠️ FreeRTOS Windows Driver Counterpart in C

See [`backend/freertos_driver_example.c`](file:///d:/Embedded/Workspace/GetSetGo/GetSetGo/winsim/backend/freertos_driver_example.c) for the complete reference C driver implementation using standard Winsock sockets (`connect`, `send`).
