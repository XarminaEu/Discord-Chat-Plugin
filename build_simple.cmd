@echo off
cd /d D:\Palworld\PalworldDiscordPlugin

if not exist build (
    echo Creating build directory...
    mkdir build
)

cd build

echo Configuring CMake...
"C:\Program Files\CMake\bin\cmake.exe" .. -G "Visual Studio 17 2022"

if errorlevel 1 (
    echo CMake configuration failed
    pause
    exit /b 1
)

echo.
echo Building plugin...
"C:\Program Files\CMake\bin\cmake.exe" --build . --config Release

if errorlevel 1 (
    echo Build failed
    pause
    exit /b 1
)

echo.
echo Build completed!
pause
