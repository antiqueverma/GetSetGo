@echo off
title Get Set Go - MCU Simulator
cd /d "%~dp0backend"
echo ============================================================
echo           GET SET GO - MCU HARDWARE SIMULATOR
echo ============================================================
echo Starting Python Backend Server (Web + TCP on 127.0.0.1:9000)...
start "Get Set Go Backend" python main.py
timeout /t 2 /nobreak >nul
echo Opening Get Set Go in Google Chrome...
if exist "C:\Program Files\Google\Chrome\Application\chrome.exe" (
    start "" "C:\Program Files\Google\Chrome\Application\chrome.exe" http://localhost:8000
) else if exist "C:\Program Files (x86)\Google\Chrome\Application\chrome.exe" (
    start "" "C:\Program Files (x86)\Google\Chrome\Application\chrome.exe" http://localhost:8000
) else (
    start http://localhost:8000
)
echo.
echo Server is running at: http://localhost:8000
echo TCP Socket listening at: 127.0.0.1:9000
echo.
echo To run the FreeRTOS driver traffic simulator test:
echo   cd backend
echo   python simulate_freertos_app.py
echo.
echo Press any key to exit this launcher window...
pause >nul
