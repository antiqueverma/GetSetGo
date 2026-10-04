"""
main.py - Get Set Go Backend Server
FastAPI Web & WebSocket server providing real-time bridge
between the React frontend and FreeRTOS Windows TCP drivers.
"""

import asyncio
import json
import logging
import os
from contextlib import asynccontextmanager
from typing import Dict, Any

from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse, JSONResponse

from connection_manager import connection_manager
from modules.gpio_module import gpio_backend
from modules.console_module import console_backend
from modules.eeprom_module import eeprom_backend
from modules.flash_module import flash_backend
from modules.adc_module import adc_backend
from modules.display_module import display_backend
from modules.timers_module import timers_backend
from modules.can_module import can_backend
from modules.canopen_module import canopen_backend

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("GetSetGo")


@asynccontextmanager
async def lifespan(app: FastAPI):
    # Register backend module handlers
    connection_manager.register_module_handler("gpio", gpio_backend.handle_packet)
    connection_manager.register_module_handler("console", console_backend.handle_packet)
    connection_manager.register_module_handler("eeprom", eeprom_backend.handle_packet)
    connection_manager.register_module_handler("flash", flash_backend.handle_packet)
    connection_manager.register_module_handler("adc", adc_backend.handle_packet)
    connection_manager.register_module_handler("display", display_backend.handle_packet)
    connection_manager.register_module_handler("timers", timers_backend.handle_packet)
    connection_manager.register_module_handler("can", can_backend.handle_packet)
    connection_manager.register_module_handler("canopen", canopen_backend.handle_packet)

    # Start TCP server on 127.0.0.1:9000
    logger.info("Initializing TCP server on 127.0.0.1:9000...")
    await connection_manager.start_tcp(host="127.0.0.1", port=9000, mode="server")

    yield

    # Clean shutdown
    logger.info("Shutting down TCP server...")
    await connection_manager.stop_tcp()


app = FastAPI(title="Get Set Go - MCU Simulator", lifespan=lifespan)

# Allow CORS for development (e.g. Vite running on port 5173 or 3000)
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


@app.get("/api/status")
async def get_status():
    """Return live status of backend, TCP socket, and modules."""
    return {
        "status": "online",
        "app": "Get Set Go",
        "mcuConnected": connection_manager.mcu_connected,
        "clientAddress": connection_manager.client_address,
        "config": {
            "ip": connection_manager.host,
            "port": connection_manager.port,
            "mode": connection_manager.mode,
            "autoReconnect": connection_manager.auto_reconnect,
        },
        "modules": {
            "gpio": gpio_backend.get_state(),
            "console": console_backend.get_state(),
            "eeprom": eeprom_backend.get_state(),
            "flash": flash_backend.get_state(),
            "adc": adc_backend.get_state(),
            "display": display_backend.get_state(),
            "timers": timers_backend.get_state(),
            "can": can_backend.get_state(),
            "canopen": canopen_backend.get_state(),
        },
    }


@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    """Bidirectional WebSocket for live packet streaming with frontend."""
    await websocket.accept()
    await connection_manager.register_websocket(websocket)

    try:
        while True:
            raw_text = await websocket.receive_text()
            try:
                data = json.loads(raw_text)

                # Check if it's a control message from frontend
                if data.get("_type") == "control":
                    action = data.get("action")
                    if action == "start_tcp":
                        ip = data.get("ip", "127.0.0.1")
                        port = int(data.get("port", 9000))
                        mode = data.get("mode", "server")
                        await connection_manager.start_tcp(ip, port, mode)
                    elif action == "stop_tcp":
                        await connection_manager.stop_tcp()
                    elif action == "update_config":
                        connection_manager.host = data.get("ip", connection_manager.host)
                        connection_manager.port = int(data.get("port", connection_manager.port))
                        connection_manager.mode = data.get("mode", connection_manager.mode)
                        connection_manager.auto_reconnect = bool(data.get("autoReconnect", True))
                        await connection_manager.broadcast_status()
                    elif action == "get_status":
                        await connection_manager.broadcast_status()
                    continue

                # Standard hardware packet from frontend -> forward to FreeRTOS over TCP
                module = data.get("module")
                if module:
                    # Update server-side module model
                    if module in connection_manager.packet_handlers:
                        connection_manager.packet_handlers[module](data)

                    # If packet is CAN, also let CANopen parse it
                    if module == "can":
                        canopen_backend.handle_packet(data)

                    # Send packet over TCP to FreeRTOS MCU driver
                    await connection_manager.send_packet_to_mcu(data)

            except json.JSONDecodeError:
                logger.warning(f"Invalid JSON received on WebSocket: {raw_text}")

    except WebSocketDisconnect:
        connection_manager.unregister_websocket(websocket)
    except Exception as e:
        logger.error(f"WebSocket error: {e}")
        connection_manager.unregister_websocket(websocket)


# Serve built frontend if dist exists
dist_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "frontend", "dist"))
if os.path.exists(dist_dir):
    assets_dir = os.path.join(dist_dir, "assets")
    if os.path.exists(assets_dir):
        app.mount("/assets", StaticFiles(directory=assets_dir), name="assets")

    @app.get("/")
    async def serve_index():
        return FileResponse(os.path.join(dist_dir, "index.html"))

    @app.get("/{full_path:path}")
    async def serve_frontend(full_path: str):
        file_path = os.path.join(dist_dir, full_path)
        if os.path.isfile(file_path):
            return FileResponse(file_path)
        return FileResponse(os.path.join(dist_dir, "index.html"))


if __name__ == "__main__":
    import uvicorn
    uvicorn.run("main:app", host="0.0.0.0", port=8000, reload=False)

