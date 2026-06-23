@echo off
setlocal

cd /d D:\Palworld\PalworldDiscordPlugin\build

echo Building plugin...
"C:\Program Files\CMake\bin\cmake.exe" --build . --config Release

if errorlevel 1 (
    echo Build failed
    pause
    exit /b 1
)

echo Build completed!
pause
