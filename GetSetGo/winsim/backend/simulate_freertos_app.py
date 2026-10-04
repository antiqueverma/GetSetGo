"""
simulate_freertos_app.py
A realistic demo script emulating a FreeRTOS Windows GCC application
communicating with the Get Set Go simulator over TCP JSON packets.

Run this script to test all 6 modules:
python simulate_freertos_app.py
"""

import socket
import json
import time
import math
import sys
import threading


HOST = "127.0.0.1"
PORT = 9000


def send_packet(sock, packet_dict):
    """Send newline-delimited JSON packet."""
    raw = json.dumps(packet_dict) + "\n"
    sock.sendall(raw.encode("utf-8"))


def receiver_thread(sock):
    """Listen for packets sent from the Get Set Go UI (e.g. user input changes)."""
    buffer = ""
    while True:
        try:
            data = sock.recv(4096)
            if not data:
                print("[FreeRTOS Sim] Connection closed by simulator.")
                break
            buffer += data.decode("utf-8", errors="replace")
            while "\n" in buffer:
                line, buffer = buffer.split("\n", 1)
                line = line.strip()
                if line:
                    try:
                        pkt = json.loads(line)
                        print(f" [FreeRTOS Driver Rx] Received from UI: {pkt}")
                    except Exception:
                        pass
        except Exception as e:
            print(f"[FreeRTOS Sim] Receiver error: {e}")
            break


def main():
    print("=" * 60)
    print(" Get Set Go - FreeRTOS Embedded Driver Simulator")
    print(f" Connecting to Simulator TCP Server at {HOST}:{PORT}...")
    print("=" * 60)

    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
        print(f"[FreeRTOS Sim] Connected successfully to {HOST}:{PORT}!")
    except Exception as e:
        print(f"[FreeRTOS Sim] Connection failed: {e}")
        print("Please ensure the Get Set Go backend is running (python main.py)")
        sys.exit(1)

    # Start background listener for packets coming back from simulator
    t = threading.Thread(target=receiver_thread, args=(s,), daemon=True)
    t.start()

    # 1. Send UART boot sequence
    print("[FreeRTOS Sim] Sending UART boot banner...")
    send_packet(s, {
        "module": "console",
        "action": "tx",
        "text": "\r\n=== FreeRTOS Kernel v10.4.3 (Windows GCC Port) ===\r\n"
    })
    time.sleep(0.3)
    send_packet(s, {
        "module": "console",
        "action": "tx",
        "text": "[SYS] Initializing peripheral drivers: GPIO, ADC, EEPROM, Flash, LCD...\r\n"
    })
    time.sleep(0.3)

    # 2. Initialize LCD 16x2
    print("[FreeRTOS Sim] Initializing 16x2 LCD...")
    send_packet(s, {
        "module": "display",
        "action": "clear",
    })
    time.sleep(0.1)
    send_packet(s, {
        "module": "display",
        "action": "write",
        "line": 0,
        "col": 0,
        "text": "FreeRTOS WinSim",
        "backlight": True
    })
    send_packet(s, {
        "module": "display",
        "action": "write",
        "line": 1,
        "col": 0,
        "text": "State: ACTIVE"
    })

    # 3. Configure GPIO directions
    print("[FreeRTOS Sim] Configuring GPIO pins...")
    send_packet(s, {
        "module": "gpio",
        "action": "config_direction",
        "port": "A",
        "pin": 5,
        "direction": "out",
        "label": "LED_HEARTBEAT"
    })
    send_packet(s, {
        "module": "gpio",
        "action": "config_direction",
        "port": "A",
        "pin": 0,
        "direction": "in",
        "label": "USER_BUTTON"
    })

    # 4. Main RTOS Task Loop Simulation
    print("\n[FreeRTOS Sim] Entering RTOS task schedule loop. Press Ctrl+C to stop.\n")
    tick = 0
    try:
        while True:
            tick += 1

            # Heartbeat LED toggle (every 1s)
            led_state = 1 if (tick % 2 == 0) else 0
            send_packet(s, {
                "module": "gpio",
                "action": "write",
                "port": "A",
                "pin": 5,
                "state": led_state
            })

            # ADC sample update (sine wave simulation)
            for ch in range(3):
                v = round(1.65 + 1.2 * math.sin(tick * 0.2 + ch), 2)
                raw = int((v / 3.3) * 4095)
                send_packet(s, {
                    "module": "adc",
                    "action": "update",
                    "channel": ch,
                    "voltage": v,
                    "raw": raw
                })

            # Update LCD Line 2 with live tick & temperature
            if tick % 3 == 0:
                temp_c = round(23.5 + 4.0 * math.sin(tick * 0.1), 1)
                send_packet(s, {
                    "module": "display",
                    "action": "write",
                    "line": 1,
                    "col": 0,
                    "text": f"T:{temp_c}C Tck:{tick:<5}"
                })

            # Write event record to EEPROM every 6 ticks
            if tick % 6 == 0:
                addr = (tick * 4) % 256
                send_packet(s, {
                    "module": "eeprom",
                    "action": "write",
                    "address": addr,
                    "data": [0xDE, 0xAD, tick & 0xFF, (tick >> 8) & 0xFF]
                })
                send_packet(s, {
                    "module": "console",
                    "action": "tx",
                    "text": f"[EEPROM] Log event saved at 0x{addr:04X} (tick {tick})\r\n"
                })

            # NOR Flash sector erase & program test every 15 ticks
            if tick % 15 == 0:
                sec_id = (tick // 15) % 8
                send_packet(s, {
                    "module": "flash",
                    "action": "sector_erase",
                    "sector": sec_id
                })
                time.sleep(0.1)
                send_packet(s, {
                    "module": "flash",
                    "action": "page_program",
                    "sector": sec_id,
                    "address": sec_id * 4096,
                    "data": [0x46, 0x52, 0x45, 0x45, 0x52, 0x54, 0x4F, 0x53] # "FREERTOS"
                })
                send_packet(s, {
                    "module": "console",
                    "action": "tx",
                    "text": f"[FLASH] Sector #{sec_id} erased & re-programmed\r\n"
                })

            time.sleep(0.5)

    except KeyboardInterrupt:
        print("\n[FreeRTOS Sim] Stopping simulation.")
    finally:
        s.close()


if __name__ == "__main__":
    main()
