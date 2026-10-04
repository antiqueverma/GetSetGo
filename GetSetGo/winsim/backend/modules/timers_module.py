"""
timers_module.py
Server-side Hardware Timers (16-bit and 32-bit) state manager.
Maintains ARR, PSC, CNT, and Capture/Compare channels.
"""

from typing import Dict, Any, List


class TimersBackendModule:
    def __init__(self):
        self.timers: Dict[str, Dict[str, Any]] = {
            "TIM1": {
                "id": "TIM1",
                "name": "Advanced Timer (16-bit)",
                "bitWidth": 16,
                "cnt": 0,
                "arr": 1000,
                "psc": 83,
                "clockMhz": 84,
                "running": True,
                "direction": "up",
                "channels": [
                    {"id": 1, "mode": "pwm1", "ccr": 250, "outputState": 1, "dutyCycle": 25.0},
                    {"id": 2, "mode": "pwm1", "ccr": 500, "outputState": 1, "dutyCycle": 50.0},
                    {"id": 3, "mode": "pwm1", "ccr": 750, "outputState": 0, "dutyCycle": 75.0},
                    {"id": 4, "mode": "output_compare", "ccr": 900, "outputState": 0, "dutyCycle": 90.0},
                ],
            },
            "TIM2": {
                "id": "TIM2",
                "name": "General Purpose Timer (32-bit)",
                "bitWidth": 32,
                "cnt": 0,
                "arr": 65535,
                "psc": 0,
                "clockMhz": 84,
                "running": True,
                "direction": "up",
                "channels": [
                    {"id": 1, "mode": "pwm1", "ccr": 32768, "outputState": 1, "dutyCycle": 50.0},
                ],
            },
        }

    def handle_packet(self, packet: Dict[str, Any]) -> Dict[str, Any]:
        timer_id = packet.get("timerId", "TIM1")
        if timer_id not in self.timers:
            self.timers[timer_id] = {
                "id": timer_id,
                "name": f"Timer {timer_id}",
                "bitWidth": 16,
                "cnt": 0,
                "arr": 1000,
                "psc": 0,
                "clockMhz": 84,
                "running": True,
                "direction": "up",
                "channels": [],
            }

        t = self.timers[timer_id]
        action = packet.get("action", "update")

        if action in ["update", "set_registers"]:
            if "arr" in packet:
                t["arr"] = int(packet["arr"])
            if "cnt" in packet:
                t["cnt"] = int(packet["cnt"])
            if "psc" in packet:
                t["psc"] = int(packet["psc"])
            if "running" in packet:
                t["running"] = bool(packet["running"])
            if "channels" in packet and isinstance(packet["channels"], list):
                t["channels"] = packet["channels"]

        elif action == "start":
            t["running"] = True
        elif action == "stop":
            t["running"] = False
        elif action == "reset":
            t["cnt"] = 0
        elif action == "set_ccr":
            ch_id = int(packet.get("channel", 1))
            ccr_val = int(packet.get("ccr", 0))
            for ch in t["channels"]:
                if ch.get("id") == ch_id:
                    ch["ccr"] = ccr_val
                    if t["arr"] > 0:
                        ch["dutyCycle"] = round((ccr_val / t["arr"]) * 100, 1)

        return t

    def get_state(self) -> Dict[str, Any]:
        return {
            "module": "timers",
            "timers": list(self.timers.values()),
        }


timers_backend = TimersBackendModule()
