@echo off
setlocal

cd /d D:\Palworld\PalworldDiscordPlugin

echo Removing old build directory...
if exist build (
    rmdir /s /q build
)

echo Creating new build directory...
mkdir build
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
echo Build completed successfully!
echo DLL location: %cd%\bin\Release\PalworldDiscordPlugin.dll
pause
