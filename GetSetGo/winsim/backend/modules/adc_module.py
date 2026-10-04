"""
adc_module.py
Server-side ADC controller tracking 8 analog channels.
Supports Vref calibration, 10/12-bit resolution, and conversion math.
"""

from typing import Dict, Any, List


class AdcBackendModule:
    def __init__(self, vref: float = 3.3, resolution: int = 12):
        self.vref = vref
        self.resolution = resolution
        self.max_raw = (1 << resolution) - 1
        self.channels: Dict[int, Dict[str, Any]] = {}

        for ch in range(8):
            self.channels[ch] = {
                "channel": ch,
                "raw": 2048,
                "voltage": 1.65,
                "name": f"CH_{ch}",
            }

    def handle_packet(self, packet: Dict[str, Any]) -> Dict[str, Any]:
        ch_num = int(packet.get("channel", 0))
        if ch_num not in self.channels:
            self.channels[ch_num] = {
                "channel": ch_num,
                "raw": 0,
                "voltage": 0.0,
                "name": f"CH_{ch_num}",
            }

        target = self.channels[ch_num]
        if "raw" in packet:
            raw = int(packet["raw"])
            target["raw"] = raw
            target["voltage"] = round((raw / self.max_raw) * self.vref, 2)
        elif "voltage" in packet:
            v = float(packet["voltage"])
            target["voltage"] = round(v, 2)
            target["raw"] = int((v / self.vref) * self.max_raw)

        return target

    def get_state(self) -> Dict[str, Any]:
        return {
            "module": "adc",
            "vref": self.vref,
            "resolution": self.resolution,
            "channels": list(self.channels.values()),
        }


adc_backend = AdcBackendModule()
