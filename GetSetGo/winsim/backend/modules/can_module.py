"""
can_module.py
Server-side CAN bus message controller.
Maintains history of transmitted and received CAN frames.
"""

from typing import Dict, Any, List
import time


class CanBackendModule:
    def __init__(self, max_history: int = 500):
        self.baud_rate = "500 kbps"
        self.history: List[Dict[str, Any]] = []
        self.max_history = max_history

    def handle_packet(self, packet: Dict[str, Any]) -> Dict[str, Any]:
        action = packet.get("action", "rx_frame")
        can_id = int(packet.get("canId", 0))
        data = packet.get("data", [])
        dlc = int(packet.get("dlc", len(data)))
        is_extended = bool(packet.get("isExtended", can_id > 0x7FF))

        entry = {
            "timestamp": time.strftime("%H:%M:%S"),
            "canId": can_id,
            "canIdHex": "0x" + hex(can_id)[2:].upper(),
            "isExtended": is_extended,
            "dlc": dlc,
            "data": data,
            "direction": packet.get("direction", "rx"),
        }

        self.history.append(entry)
        if len(self.history) > self.max_history:
            self.history.pop(0)

        return entry

    def get_state(self) -> Dict[str, Any]:
        return {
            "module": "can",
            "baudRate": self.baud_rate,
            "frameCount": len(self.history),
            "recent": self.history[-20:],
        }


can_backend = CanBackendModule()
