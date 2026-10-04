"""
gpio_module.py
Server-side GPIO state tracker and validator.
Handles Ports A, B, C, D, E, F, G, H with 16 pins each (GPIOA - GPIOH, 16-bit).
Supports direction (in/out), pin state (0/1), and pull resistors.
"""

from typing import Dict, Any


class GpioBackendModule:
    def __init__(self):
        # Key: "A0".."A15", "B0".."B15", ... "H0".."H15"
        self.pins: Dict[str, Dict[str, Any]] = {}
        self.ports = ["A", "B", "C", "D", "E", "F", "G", "H"]

        # Pre-initialize all 8 ports with 16 pins each (128 pins total)
        for port in self.ports:
            for pin in range(16):
                pin_id = f"{port}{pin}"
                self.pins[pin_id] = {
                    "port": port,
                    "pin": pin,
                    "state": 0,
                    "direction": "out" if pin < 8 else "in",
                    "pull": "up" if pin >= 8 else "none",
                }

    def handle_packet(self, packet: Dict[str, Any]) -> Dict[str, Any]:
        """Process incoming GPIO packet and update state."""
        port = str(packet.get("port", "A")).upper()
        pin = int(packet.get("pin", 0))
        pin_id = f"{port}{pin}"

        if pin_id not in self.pins:
            self.pins[pin_id] = {
                "port": port,
                "pin": pin,
                "state": 0,
                "direction": "out",
                "pull": "none",
            }

        target = self.pins[pin_id]
        if "state" in packet:
            target["state"] = 1 if packet["state"] else 0
        if "direction" in packet:
            target["direction"] = packet["direction"]
        if "pull" in packet:
            target["pull"] = packet["pull"]

        return target

    def get_state(self) -> Dict[str, Any]:
        return {
            "module": "gpio",
            "ports": self.ports,
            "pinCountPerPort": 16,
            "totalPins": len(self.pins),
            "pins": self.pins,
        }


gpio_backend = GpioBackendModule()

