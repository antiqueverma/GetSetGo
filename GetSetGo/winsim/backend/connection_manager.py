"""
connection_manager.py
Handles TCP connection to FreeRTOS MCU driver (as TCP Server or Client)
and relays JSON packets bidirectionally to/from WebSocket browser clients.
"""

import asyncio
import json
import logging
from typing import Dict, Set, Optional, Any, Callable

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("ConnectionManager")


class ConnectionManager:
    def __init__(self):
        self.tcp_server: Optional[asyncio.Server] = None
        self.tcp_reader: Optional[asyncio.StreamReader] = None
        self.tcp_writer: Optional[asyncio.StreamWriter] = None
        self.tcp_client_task: Optional[asyncio.Task] = None
        
        self.host: str = "127.0.0.1"
        self.port: int = 9000
        self.mode: str = "server"  # 'server' or 'client'
        self.auto_reconnect: bool = True
        
        self.mcu_connected: bool = False
        self.client_address: Optional[str] = None
        
        self.websocket_clients: Set[Any] = set()
        self.packet_handlers: Dict[str, Callable] = {}
        
        # Statistics
        self.bytes_received: int = 0
        self.bytes_sent: int = 0
        self.packets_received: int = 0
        self.packets_sent: int = 0

    def register_module_handler(self, module_name: str, handler: Callable):
        """Register a backend handler for a specific module."""
        self.packet_handlers[module_name] = handler

    # ---------------- WebSocket Management ----------------
    async def register_websocket(self, websocket: Any):
        self.websocket_clients.add(websocket)
        logger.info(f"WebSocket client connected. Total clients: {len(self.websocket_clients)}")
        # Send current status immediately
        await self.broadcast_status()

    def unregister_websocket(self, websocket: Any):
        self.websocket_clients.discard(websocket)
        logger.info(f"WebSocket client disconnected. Total clients: {len(self.websocket_clients)}")

    async def broadcast_to_websockets(self, message: str):
        """Send message string to all connected browser clients."""
        if not self.websocket_clients:
            return
        dead_clients = set()
        for ws in self.websocket_clients:
            try:
                await ws.send_text(message)
            except Exception as e:
                logger.warning(f"Error sending to WebSocket: {e}")
                dead_clients.add(ws)
        self.websocket_clients.difference_update(dead_clients)

    async def broadcast_status(self):
        """Broadcast live status to all frontends."""
        status_msg = json.dumps({
            "_type": "server_status",
            "mcuConnected": self.mcu_connected,
            "clientAddress": self.client_address,
            "config": {
                "ip": self.host,
                "port": self.port,
                "mode": self.mode,
                "autoReconnect": self.auto_reconnect,
            },
            "stats": {
                "bytesReceived": self.bytes_received,
                "bytesSent": self.bytes_sent,
                "packetsReceived": self.packets_received,
                "packetsSent": self.packets_sent,
            }
        })
        await self.broadcast_to_websockets(status_msg)

    # ---------------- TCP Server & Client Management ----------------
    async def start_tcp(self, host: str = "127.0.0.1", port: int = 9000, mode: str = "server"):
        """Start TCP Server or Client depending on configured mode."""
        await self.stop_tcp()
        self.host = host
        self.port = port
        self.mode = mode

        if self.mode == "server":
            try:
                self.tcp_server = await asyncio.start_server(
                    self._handle_incoming_mcu_client, self.host, self.port
                )
                logger.info(f"TCP Server listening on {self.host}:{self.port}")
                await self.broadcast_status()
            except Exception as e:
                logger.error(f"Failed to start TCP server on {self.host}:{self.port}: {e}")
                self.mcu_connected = False
                await self.broadcast_status()
        else:
            # Client mode (connect to FreeRTOS simulator server)
            self.tcp_client_task = asyncio.create_task(self._client_loop())

    async def stop_tcp(self):
        """Stop active TCP server or connection."""
        if self.tcp_server:
            self.tcp_server.close()
            await self.tcp_server.wait_closed()
            self.tcp_server = None
            logger.info("TCP Server stopped.")

        if self.tcp_client_task:
            self.tcp_client_task.cancel()
            self.tcp_client_task = None

        if self.tcp_writer:
            try:
                self.tcp_writer.close()
                await self.tcp_writer.wait_closed()
            except Exception:
                pass
            self.tcp_writer = None
            self.tcp_reader = None

        self.mcu_connected = False
        self.client_address = None
        await self.broadcast_status()

    async def _handle_incoming_mcu_client(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
        """Handle incoming connection from FreeRTOS Windows process."""
        addr = writer.get_extra_info("peername")
        self.client_address = f"{addr[0]}:{addr[1]}" if addr else "unknown"
        self.tcp_reader = reader
        self.tcp_writer = writer
        self.mcu_connected = True
        logger.info(f"FreeRTOS MCU connected from {self.client_address}")
        await self.broadcast_status()

        try:
            await self._process_stream(reader)
        except asyncio.CancelledError:
            pass
        except Exception as e:
            logger.error(f"Error handling MCU stream: {e}")
        finally:
            logger.info(f"FreeRTOS MCU disconnected from {self.client_address}")
            self.mcu_connected = False
            self.client_address = None
            self.tcp_writer = None
            self.tcp_reader = None
            await self.broadcast_status()

    async def _client_loop(self):
        """Continually attempt to connect to MCU server if in client mode."""
        while True:
            try:
                logger.info(f"Attempting TCP connection to MCU server {self.host}:{self.port}...")
                reader, writer = await asyncio.open_connection(self.host, self.port)
                self.client_address = f"{self.host}:{self.port}"
                self.tcp_reader = reader
                self.tcp_writer = writer
                self.mcu_connected = True
                logger.info(f"Connected to MCU server at {self.client_address}")
                await self.broadcast_status()

                await self._process_stream(reader)
            except asyncio.CancelledError:
                break
            except Exception as e:
                logger.warning(f"TCP connection failed ({e}). Retrying in 3s...")
                self.mcu_connected = False
                await self.broadcast_status()
                await asyncio.sleep(3)

    async def _process_stream(self, reader: asyncio.StreamReader):
        """Read and parse framed JSON packets from the stream."""
        buffer = ""
        while not reader.at_eof():
            data = await reader.read(4096)
            if not data:
                break
            self.bytes_received += len(data)
            text = data.decode("utf-8", errors="replace")
            buffer += text

            # Process JSON objects (supports newline-delimited or brace-matched JSON)
            while True:
                # 1. Try newline-delimited first
                if "\n" in buffer:
                    line, remainder = buffer.split("\n", 1)
                    buffer = remainder
                    line = line.strip()
                    if line:
                        await self._handle_packet_string(line)
                elif "{" in buffer and "}" in buffer:
                    start = buffer.find("{")
                    # Try finding matching bracket
                    depth = 0
                    end = -1
                    for idx in range(start, len(buffer)):
                        if buffer[idx] == "{":
                            depth += 1
                        elif buffer[idx] == "}":
                            depth -= 1
                            if depth == 0:
                                end = idx
                                break
                    if end != -1:
                        packet_str = buffer[start:end+1]
                        buffer = buffer[end+1:]
                        await self._handle_packet_string(packet_str)
                    else:
                        break
                else:
                    break

    async def _handle_packet_string(self, raw_str: str):
        """Parse JSON packet from FreeRTOS and broadcast to frontends."""
        try:
            packet = json.loads(raw_str)
            self.packets_received += 1
            
            # Module specific processing if registered
            module = packet.get("module")
            if module and module in self.packet_handlers:
                try:
                    self.packet_handlers[module](packet)
                except Exception as ex:
                    logger.error(f"Error in module handler for {module}: {ex}")

            # Relay to browser frontends
            await self.broadcast_to_websockets(json.dumps(packet))
        except json.JSONDecodeError as err:
            logger.warning(f"Invalid JSON received from MCU: '{raw_str}' ({err})")

    # ---------------- Send Packet to FreeRTOS Driver ----------------
    async def send_packet_to_mcu(self, packet_dict: dict) -> bool:
        """Send a JSON packet to the connected FreeRTOS Windows process."""
        if not self.tcp_writer or not self.mcu_connected:
            logger.warning("Cannot send packet: FreeRTOS MCU is not connected over TCP.")
            return False

        try:
            raw = json.dumps(packet_dict) + "\n"
            data = raw.encode("utf-8")
            self.tcp_writer.write(data)
            await self.tcp_writer.drain()
            self.bytes_sent += len(data)
            self.packets_sent += 1
            return True
        except Exception as e:
            logger.error(f"Failed to send packet to MCU: {e}")
            return False


# Global connection manager instance
connection_manager = ConnectionManager()
