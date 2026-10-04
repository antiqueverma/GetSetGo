"""
console_module.py
Server-side Console/UART message buffer and state tracker.
"""

from typing import Dict, Any, List
import time


class ConsoleBackendModule:
    def __init__(self, max_history: int = 500):
        self.baud_rate: int = 115200
        self.history: List[Dict[str, Any]] = []
        self.max_history = max_history

    def handle_packet(self, packet: Dict[str, Any]) -> Dict[str, Any]:
        """Record serial output or command."""
        text = str(packet.get("text", ""))
        direction = packet.get("direction", "tx")
        entry = {
            "timestamp": time.strftime("%H:%M:%S"),
            "direction": direction,
            "text": text,
        }
        self.history.append(entry)
        if len(self.history) > self.max_history:
            self.history.pop(0)

        if "baudRate" in packet:
            self.baud_rate = int(packet["baudRate"])

        return entry

    def get_state(self) -> Dict[str, Any]:
        return {
            "module": "console",
            "baudRate": self.baud_rate,
            "history": self.history[-50:],
        }


console_backend = ConsoleBackendModule()
