"""
canopen_module.py
Server-side CANopen layer (CiA 301).
Maintains Node Status Matrix (1 to 127) and object dictionary telemetry.
"""

from typing import Dict, Any, List
import time


class CanopenBackendModule:
    def __init__(self):
        self.nodes: Dict[int, Dict[str, Any]] = {}
        for i in range(1, 9):
            self.nodes[i] = {
                "nodeId": i,
                "state": "OPERATIONAL" if i == 1 else "UNKNOWN",
                "lastSeen": time.time(),
            }

    def handle_packet(self, packet: Dict[str, Any]) -> Dict[str, Any]:
        can_id = int(packet.get("canId", 0))
        data = packet.get("data", [])

        # Heartbeat check (0x700 + NodeID)
        if 0x701 <= can_id <= 0x77F:
            node_id = can_id - 0x700
            st_val = data[0] if len(data) > 0 else 5
            st_map = {0: "BOOTUP", 4: "STOPPED", 5: "OPERATIONAL", 127: "PRE_OPERATIONAL"}
            state_str = st_map.get(st_val, "UNKNOWN")

            self.nodes[node_id] = {
                "nodeId": node_id,
                "state": state_str,
                "lastSeen": time.time(),
            }
            return self.nodes[node_id]

        return {}

    def get_state(self) -> Dict[str, Any]:
        return {
            "module": "canopen",
            "nodes": list(self.nodes.values()),
        }


canopen_backend = CanopenBackendModule()
