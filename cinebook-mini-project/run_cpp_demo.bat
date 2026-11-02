@echo off
title CineBook - C++ Data Structures Engine (College Viva Demo)
color 0B
cls
echo ======================================================================
echo    CINEBOOK: C++ DATA STRUCTURES ENGINE & VIVA DEMONSTRATION
echo ======================================================================
echo.

if not exist "%~dp0backend\cpp\cinebook_ds.exe" (
    echo [INFO] cinebook_ds.exe not found. Compiling with g++...
    cd /d "%~dp0backend\cpp"
    g++ -std=c++14 -O2 data_structures.cpp main.cpp -o cinebook_ds.exe
    if %ERRORLEVEL% NEQ 0 (
        echo [ERROR] Failed to compile C++ engine. Please ensure g++ is installed.
        pause
        exit /b 1
    )
    echo [OK] Compiled successfully!
    echo.
)

cd /d "%~dp0backend\cpp"
cinebook_ds.exe --demo
pause
