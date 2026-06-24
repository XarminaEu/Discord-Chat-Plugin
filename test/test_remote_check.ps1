# Manual test: run plugin WITHOUT skipping remote copyright check
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$dll = Join-Path $root "build\bin\Release\PalworldDiscordPlugin.dll"
$testDir = $PSScriptRoot

$configDir = Join-Path $testDir "PalworldDiscordConfig"
New-Item -ItemType Directory -Force -Path $configDir | Out-Null
Copy-Item (Join-Path $testDir "test_config.json") (Join-Path $configDir "config.json") -Force
Remove-Item (Join-Path $testDir "PalworldDiscordPlugin_test.log") -ErrorAction SilentlyContinue

Push-Location $testDir
[System.Environment]::CurrentDirectory = $testDir

# Ensure the skip variable is NOT set
Remove-Item Env:\PALDISCORDPLUGIN_SKIP_REMOTE_CHECK -ErrorAction SilentlyContinue

Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Native {
    [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Ansi)]
    public static extern IntPtr LoadLibrary(string path);
    [DllImport("kernel32.dll", SetLastError=true)]
    public static extern bool FreeLibrary(IntPtr hModule);
    [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Ansi)]
    public static extern IntPtr GetProcAddress(IntPtr hModule, string procName);
}
public delegate int StartBridgeDelegate(IntPtr arg);
"@

$h = [Native]::LoadLibrary($dll)
if ($h -eq [IntPtr]::Zero) {
    Write-Host "LoadLibrary failed" -ForegroundColor Red
    exit 1
}

$proc = [Native]::GetProcAddress($h, "StartBridge")
if ($proc -eq [IntPtr]::Zero) {
    Write-Host "StartBridge not found" -ForegroundColor Red
    exit 1
}

$startBridge = [System.Runtime.InteropServices.Marshal]::GetDelegateForFunctionPointer($proc, [StartBridgeDelegate])
$startBridge.Invoke([IntPtr]::Zero) | Out-Null

Write-Host "StartBridge called. Waiting 5 seconds..." -ForegroundColor Cyan
Start-Sleep -Seconds 5

[Native]::FreeLibrary($h) | Out-Null
Pop-Location

$logFile = Join-Path $testDir "PalworldDiscordPlugin_test.log"
if (Test-Path $logFile) {
    Write-Host "Log output:" -ForegroundColor Cyan
    Get-Content $logFile | Select-Object -Last 30
} else {
    Write-Host "No log file found" -ForegroundColor Red
}
