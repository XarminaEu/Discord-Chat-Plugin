# Palworld Discord Plugin - PowerShell Build Script

Write-Host "================================================================================" -ForegroundColor Cyan
Write-Host "Palworld Discord Plugin - Build Script" -ForegroundColor Cyan
Write-Host "================================================================================" -ForegroundColor Cyan
Write-Host ""

# Check CMake
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    Write-Host "ERROR: CMake not found in PATH" -ForegroundColor Red
    Write-Host "Please install CMake from https://cmake.org/download/" -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit 1
}

# Check Visual Studio Build Tools
$vsPath = "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vsPath)) {
    Write-Host "ERROR: Visual Studio Build Tools not found" -ForegroundColor Red
    Write-Host "Please install from https://visualstudio.microsoft.com/visual-cpp-build-tools/" -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host "[*] Detected tools:" -ForegroundColor Green
cmake --version | Select-Object -First 1
Write-Host ""

# Create build directory
if (-not (Test-Path "build")) {
    Write-Host "[*] Creating build directory..." -ForegroundColor Green
    New-Item -ItemType Directory -Path "build" | Out-Null
}

Set-Location build

Write-Host "[*] Configuring CMake..." -ForegroundColor Green
cmake .. -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: CMake configuration failed" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host ""
Write-Host "[*] Building plugin..." -ForegroundColor Green
cmake --build . --config Release

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Build failed" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host ""
Write-Host "================================================================================" -ForegroundColor Green
Write-Host "Build completed successfully!" -ForegroundColor Green
Write-Host "================================================================================" -ForegroundColor Green
Write-Host ""

$dllPath = Join-Path (Get-Location) "bin\Release\PalworldDiscordPlugin.dll"
Write-Host "Output DLL: $dllPath" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "1. Copy the DLL to: C:\Program Files\Palworld\Binaries\Win64\" -ForegroundColor White
Write-Host "2. Copy config.json to the same directory" -ForegroundColor White
Write-Host "3. Edit config.json with your Discord settings" -ForegroundColor White
Write-Host "4. Restart the Palworld server" -ForegroundColor White
Write-Host ""

Read-Host "Press Enter to exit"
