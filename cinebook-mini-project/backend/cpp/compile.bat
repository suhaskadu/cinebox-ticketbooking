@echo off
title Compile CineBook C++ Data Structures Engine
color 0A
cls
echo ======================================================================
echo    CINEBOOK: COMPILING C++ DATA STRUCTURES ENGINE (MinGW / GCC)
echo ======================================================================
echo.

where g++ >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] g++ compiler not found in PATH!
    echo Please make sure MinGW or GCC is installed.
    pause
    exit /b 1
)

echo [1/2] Compiling main engine (cinebook_ds.exe)...
g++ -std=c++14 -O2 data_structures.cpp main.cpp -o cinebook_ds.exe
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Compilation failed!
    pause
    exit /b 1
)

echo [2/2] Compiling standalone test runner (test_ds.exe)...
g++ -std=c++14 -O2 data_structures.cpp test_ds.cpp -o test_ds.exe
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Test suite compilation failed!
    pause
    exit /b 1
)

echo.
echo ======================================================================
echo  [SUCCESS] Both C++ executables compiled successfully!
echo    1. cinebook_ds.exe (Interactive CLI, Viva Demo, JSON IPC Bridge)
echo    2. test_ds.exe     (Standalone Unit Test Runner)
echo ======================================================================
echo.
pause
