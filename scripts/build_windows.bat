@echo off
REM Windows Build Script for MTProxy Advanced Systems

echo Building MTProxy Advanced Systems for Windows...

REM Create build directory if it doesn't exist
if not exist build mkdir build
cd build

REM Try to compile with GCC (assuming MinGW or similar toolchain)
if defined CC (
    set COMPILER=%CC%
) else (
    set COMPILER=gcc
)

echo Using compiler: %COMPILER%

REM Compile the test file
%COMPILER% ..\system\windows-compatibility-test.c ^
    ..\system\diagnostic\advanced-diagnostic-system.c ^
    ..\system\monitoring\real-time-monitoring-dashboard.c ^
    ..\system\debugging\intelligent-debugging-framework.c ^
    ..\system\optimization\performance-correlation-engine.c ^
    ..\system\optimization\resource-optimization-manager.c ^
    ..\system\monitoring\system-health-monitor.c ^
    ..\system\integration\system-integration-coordinator.c ^
    ..\system\integration\mtproxy-integration-layer.c ^
    ..\system\config\unified-config-manager.c ^
    ..\system\config\advanced-systems-defaults.c ^
    -o mtproxy_advanced_systems.exe -std=c99 -Wall -Wextra

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build completed successfully!
    echo You can run the executable with: mtproxy_advanced_systems.exe
) else (
    echo.
    echo Build failed. Please check compiler installation.
    echo Ensure you have GCC or Clang installed and in your PATH.
)

cd ..
pause