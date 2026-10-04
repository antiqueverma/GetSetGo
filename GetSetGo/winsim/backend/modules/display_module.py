"""
display_module.py
Server-side 16x2 HD44780 LCD display state manager.
Maintains 2 rows x 16 characters DDRAM buffer, cursor, and backlight state.
"""

from typing import Dict, Any, List


class DisplayBackendModule:
    def __init__(self):
        self.lines: List[str] = ["FreeRTOS WinSim ", "System Ready... "]
        self.cursor = {"row": 0, "col": 0, "visible": True, "blink": True}
        self.backlight = True
        self.contrast = 90

    def handle_packet(self, packet: Dict[str, Any]) -> Dict[str, Any]:
        action = packet.get("action", "write")

        if action == "clear":
            self.lines = [" " * 16, " " * 16]
            self.cursor["row"] = 0
            self.cursor["col"] = 0
            return {"status": "cleared"}

        elif action in ["write", "print"]:
            line_idx = int(packet.get("line", 0))
            col = int(packet.get("col", 0))
            text = str(packet.get("text", ""))

            if line_idx in [0, 1]:
                curr = self.lines[line_idx].ljust(16, " ")
                before = curr[:col]
                after = curr[col + len(text):]
                merged = (before + text + after)[:16].ljust(16, " ")
                self.lines[line_idx] = merged

            if "backlight" in packet:
                self.backlight = bool(packet["backlight"])

            return {"lines": self.lines}

        elif action == "backlight":
            self.backlight = bool(packet.get("enabled", packet.get("state", True)))
            return {"backlight": self.backlight}

        return {}

    def get_state(self) -> Dict[str, Any]:
        return {
            "module": "display",
            "lines": self.lines,
            "cursor": self.cursor,
            "backlight": self.backlight,
            "contrast": self.contrast,
        }


display_backend = DisplayBackendModule()
