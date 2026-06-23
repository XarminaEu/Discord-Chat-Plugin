@echo off
REM Palworld Discord Plugin - Build Script
REM This script builds the plugin using CMake

setlocal enabledelayedexpansion

echo.
echo ================================================================================
echo Palworld Discord Plugin - Build Script
echo ================================================================================
echo.

REM Check if CMake is installed
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: CMake not found in PATH
    echo Please install CMake from https://cmake.org/download/
    pause
    exit /b 1
)

REM Check if Visual Studio Build Tools are installed
if not exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
    echo ERROR: Visual Studio Build Tools not found
    echo Please install from https://visualstudio.microsoft.com/visual-cpp-build-tools/
    pause
    exit /b 1
)

echo [*] Detected tools:
cmake --version | findstr /R "cmake version"
echo.

REM Create build directory if it doesn't exist
if not exist "build" (
    echo [*] Creating build directory...
    mkdir build
)

cd build

echo [*] Configuring CMake...
cmake .. -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release

if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)

echo.
echo [*] Building plugin...
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo ================================================================================
echo Build completed successfully!
echo ================================================================================
echo.
echo Output DLL: %cd%\bin\Release\PalworldDiscordPlugin.dll
echo.
echo Next steps:
echo 1. Copy the DLL to: C:\Program Files\Palworld\Binaries\Win64\
echo 2. Copy config.json to the same directory
echo 3. Edit config.json with your Discord settings
echo 4. Restart the Palworld server
echo.
pause
