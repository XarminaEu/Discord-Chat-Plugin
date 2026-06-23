@echo off
REM Palworld Discord Plugin - Direct Build Script
REM Uses full path to CMake

setlocal enabledelayedexpansion

echo.
echo ================================================================================
echo Palworld Discord Plugin - Build Script
echo ================================================================================
echo.

set CMAKE_PATH=C:\Program Files\CMake\bin\cmake.exe
set VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat

REM Check if CMake exists
if not exist "%CMAKE_PATH%" (
    echo ERROR: CMake not found at %CMAKE_PATH%
    echo Please install CMake from https://cmake.org/download/
    pause
    exit /b 1
)

REM Check if Visual Studio Build Tools exist
if not exist "%VS_PATH%" (
    echo ERROR: Visual Studio Build Tools not found
    echo Please install from https://visualstudio.microsoft.com/visual-cpp-build-tools/
    pause
    exit /b 1
)

echo [*] CMake found: %CMAKE_PATH%
"%CMAKE_PATH%" --version
echo.

REM Create build directory if it doesn't exist
if not exist "build" (
    echo [*] Creating build directory...
    mkdir build
)

cd build

echo [*] Configuring CMake...
"%CMAKE_PATH%" .. -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release

if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)

echo.
echo [*] Building plugin (this may take a few minutes)...
"%CMAKE_PATH%" --build . --config Release

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

REM Find the DLL
for /r "." %%f in (PalworldDiscordPlugin.dll) do (
    echo Output DLL: %%f
    set DLL_PATH=%%f
)

echo.
echo Next steps:
echo 1. Copy the DLL to: C:\Program Files\Palworld\Binaries\Win64\
echo 2. Copy config.json to the same directory
echo 3. Edit config.json with your Discord settings
echo 4. Restart the Palworld server
echo.
pause
