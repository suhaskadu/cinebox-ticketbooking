@echo off
title CineBook - Movie Ticket Booking System (C++ Engine + Web Dashboard)
color 0B
cls
echo ======================================================================
echo    CINEBOOK: MOVIE TICKET BOOKING SYSTEM - MINI PROJECT
echo    C++ Native Data Structures Engine (Matrix, HashMap, Heap, Queue, DLL)
echo    Theme: Modern Cinema (Navy, Purple, Red Accents)
echo    Includes: 4 Screens + 5-Minute Seat Lock + SQLite DB + C++ DS Engine
echo ======================================================================
echo.

REM Step 1: Ensure C++ Engine is compiled
if not exist "%~dp0backend\cpp\cinebook_ds.exe" (
    echo [INFO] Compiling C++ Data Structures Engine with g++...
    where g++ >nul 2>&1
    if %ERRORLEVEL% EQU 0 (
        cd /d "%~dp0backend\cpp"
        g++ -std=c++14 -O2 data_structures.cpp main.cpp -o cinebook_ds.exe
        cd /d "%~dp0"
        echo [OK] C++ Engine compiled successfully!
    ) else (
        echo [NOTE] g++ not found on PATH. Will use Python bridge.
    )
    echo.
) else (
    echo [OK] Native C++ Engine verified: backend\cpp\cinebook_ds.exe
    echo.
)

REM Step 2: Check for Python
python --version >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [OK] Python detected. Starting CineBook Server with Native C++ Engine...
    echo.
    echo Web Dashboard:  http://localhost:8000
    echo C++ Terminal:   run_cpp_demo.bat (Double-click for College Viva)
    echo.
    echo Press Ctrl+C in this terminal window anytime to stop the server.
    echo.
    start http://localhost:8000
    python backend\server.py
    pause
    exit /b
)

REM Step 3: Check for Node.js fallback
node --version >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [OK] Node.js detected. Starting CineBook Server...
    echo.
    echo Web Dashboard: http://localhost:8000
    echo.
    start http://localhost:8000
    node backend\server.js
    pause
    exit /b
)

REM Step 4: Fallback if neither runtime is on PATH
echo [NOTICE] Neither Python nor Node was found on PATH.
echo Opening standalone web interface in your default browser...
start frontend\index.html
pause
