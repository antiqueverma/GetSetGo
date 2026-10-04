@echo off
title Get Set Go - FreeRTOS Driver Simulator Client
cd /d "%~dp0\backend"
echo ============================================================
echo   Running FreeRTOS Driver Emulation Client on TCP:9000
echo ============================================================
python simulate_freertos_app.py
pause
