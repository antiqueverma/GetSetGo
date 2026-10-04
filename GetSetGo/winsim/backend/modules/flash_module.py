"""
flash_module.py
Server-side Flash Memory simulation (NOR Flash sectors/blocks).
Enforces real Flash physics: bits can only be changed from 1 to 0 (AND operation).
Only a Sector Erase operation restores bits to 1 (0xFF).
"""

from typing import Dict, Any, List


class FlashBackendModule:
    def __init__(self, num_sectors: int = 8, sector_size: int = 4096):
        self.num_sectors = num_sectors
        self.sector_size = sector_size
        self.sectors: List[Dict[str, Any]] = []

        for i in range(num_sectors):
            # 256 bytes preview per sector
            data = bytearray([0xFF] * 256)
            if i == 0:
                sig = b"GSG_FLASH_V1"
                data[:len(sig)] = sig

            self.sectors.append({
                "id": i,
                "address": i * sector_size,
                "size": sector_size,
                "eraseCycles": 12 if i == 0 else 1,
                "data": data,
            })

    def handle_packet(self, packet: Dict[str, Any]) -> Dict[str, Any]:
        action = packet.get("action", "write")

        if action in ["sector_erase", "erase"]:
            sector_id = int(packet.get("sector", 0))
            if 0 <= sector_id < len(self.sectors):
                sec = self.sectors[sector_id]
                sec["eraseCycles"] += 1
                sec["data"] = bytearray([0xFF] * 256)
                return {"status": "erased", "sector": sector_id}

        elif action in ["page_program", "write"]:
            addr = int(packet.get("address", 0))
            sec_id = addr // self.sector_size
            offset = addr % self.sector_size
            data = packet.get("data", [])
            if isinstance(data, int):
                data = [data]

            if 0 <= sec_id < len(self.sectors):
                sec = self.sectors[sec_id]
                for i, b in enumerate(data):
                    target_idx = offset + i
                    if target_idx < len(sec["data"]):
                        # Flash bitwise AND
                        sec["data"][target_idx] = sec["data"][target_idx] & (int(b) & 0xFF)

                return {"status": "programmed", "sector": sec_id, "offset": offset}

        return {}

    def get_state(self) -> Dict[str, Any]:
        return {
            "module": "flash",
            "sectors": [
                {
                    "id": s["id"],
                    "address": s["address"],
                    "size": s["size"],
                    "eraseCycles": s["eraseCycles"],
                    "data": list(s["data"]),
                }
                for s in self.sectors
            ],
        }


flash_backend = FlashBackendModule()
