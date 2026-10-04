"""
eeprom_module.py
Server-side EEPROM memory simulation (512 or 1024 bytes).
Supports read, write, fill, and persistent in-memory byte buffer.
"""

from typing import Dict, Any, List


class EepromBackendModule:
    def __init__(self, size: int = 512):
        self.size = size
        self.memory = bytearray([0xFF] * size)
        # Signature
        sig = b"GETSETGO_EEPROM"
        self.memory[:len(sig)] = sig
        self.write_count = 0
        self.last_address = None

    def handle_packet(self, packet: Dict[str, Any]) -> Dict[str, Any]:
        action = packet.get("action", "write")
        
        if action == "fill":
            fill_val = int(packet.get("value", 0xFF)) & 0xFF
            self.memory = bytearray([fill_val] * self.size)
            self.write_count += self.size
            return {"status": "filled", "value": fill_val}

        elif action in ["write", "update"]:
            addr = int(packet.get("address", 0))
            data = packet.get("data", [])
            if isinstance(data, int):
                data = [data]
            elif isinstance(data, str):
                # Hex string
                clean = data.replace(" ", "")
                data = [int(clean[i:i+2], 16) for i in range(0, len(clean), 2)]

            for i, byte_val in enumerate(data):
                target_addr = addr + i
                if 0 <= target_addr < self.size:
                    self.memory[target_addr] = int(byte_val) & 0xFF
                    self.write_count += 1
                    self.last_address = target_addr

            return {"status": "written", "address": addr, "length": len(data)}

        return {}

    def get_state(self) -> Dict[str, Any]:
        return {
            "module": "eeprom",
            "size": self.size,
            "data": list(self.memory),
            "writeCount": self.write_count,
            "lastAddress": self.last_address,
        }


eeprom_backend = EepromBackendModule()
