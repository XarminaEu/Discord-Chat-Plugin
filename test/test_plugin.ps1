# Runtime test for PalworldDiscordPlugin
# Loads the DLL via StartBridge and verifies the file bridge works.
# The plugin does NOT start an HTTP server.

$ErrorActionPreference = "Stop"

# Skip remote copyright check during local testing.
$env:PALDISCORDPLUGIN_SKIP_REMOTE_CHECK = "1"

$root = Split-Path -Parent $PSScriptRoot
$dll = Join-Path $root "build\bin\Release\PalworldDiscordPlugin.dll"
$testDir = $PSScriptRoot

if (-not (Test-Path $dll)) {
    Write-Host "ERROR: DLL not found at $dll" -ForegroundColor Red
    exit 1
}

# Place config.json in the test working directory
Copy-Item (Join-Path $testDir "test_config.json") (Join-Path $testDir "config.json") -Force

# Clean any leftover bridge files and logs from a previous run
Remove-Item (Join-Path $testDir "bridge_in.txt") -ErrorAction SilentlyContinue
Remove-Item (Join-Path $testDir "bridge_out.txt") -ErrorAction SilentlyContinue
Remove-Item (Join-Path $testDir "PalworldDiscordPlugin_test.log") -ErrorAction SilentlyContinue

Push-Location $testDir
[System.Environment]::CurrentDirectory = $testDir

# P/Invoke LoadLibrary / GetProcAddress / FreeLibrary
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

Write-Host "[*] Loading plugin DLL..." -ForegroundColor Cyan
$h = [Native]::LoadLibrary($dll)
if ($h -eq [IntPtr]::Zero) {
    Write-Host "ERROR: LoadLibrary failed" -ForegroundColor Red
    Pop-Location
    exit 1
}

# The plugin initializes via the StartBridge export, not DllMain.
$proc = [Native]::GetProcAddress($h, "StartBridge")
if ($proc -eq [IntPtr]::Zero) {
    Write-Host "ERROR: StartBridge export not found" -ForegroundColor Red
    Pop-Location
    exit 1
}

Write-Host "[*] Calling StartBridge export..." -ForegroundColor Cyan
$startBridge = [System.Runtime.InteropServices.Marshal]::GetDelegateForFunctionPointer($proc, [StartBridgeDelegate])
$startBridge.Invoke([IntPtr]::Zero) | Out-Null

Write-Host "[*] Waiting for plugin to initialize..." -ForegroundColor Green
Start-Sleep -Seconds 2

$pass = $true

# Test 1: Verify the plugin started successfully (no HTTP server, but file bridge active)
try {
    $logFile = Join-Path $testDir "PalworldDiscordPlugin_test.log"
    if (Test-Path $logFile) {
        $log = Get-Content $logFile -Raw
        if ($log -match "Plugin loaded and ready") {
            Write-Host "[Test 1] Plugin initialized successfully" -ForegroundColor Green
        } else {
            $pass = $false
            Write-Host "[Test 1] FAILED: plugin did not report ready" -ForegroundColor Red
        }
    } else {
        $pass = $false
        Write-Host "[Test 1] FAILED: log file not found" -ForegroundColor Red
    }
} catch {
    $pass = $false
    Write-Host "[Test 1] FAILED: $_" -ForegroundColor Red
}

# Test 2: game -> Discord: writing to outgoing bridge file triggers OnGameChatMessage
try {
    $outFile = Join-Path $testDir "bridge_out.txt"
    "chat|Alice|Hi from game" | Out-File -FilePath $outFile -Append -Encoding ascii
    # Wait for FileBridge watcher poll (500ms interval) + processing
    Start-Sleep -Milliseconds 1200
} catch {
    $pass = $false
    Write-Host "[Test 2] FAILED: $_" -ForegroundColor Red
}

Write-Host "[*] Unloading DLL..." -ForegroundColor Cyan
[Native]::FreeLibrary($h) | Out-Null

# Now the logger has closed the file; verify Test 2 log line.
try {
    $logFile = Join-Path $testDir "PalworldDiscordPlugin_test.log"
    if (Test-Path $logFile) {
        $log = Get-Content $logFile -Raw
        if ($log -match "Game chat: Alice: Hi from game") {
            Write-Host "[Test 2] Outgoing bridge file forwarded to game-chat handler" -ForegroundColor Green
        } else {
            $pass = $false
            Write-Host "[Test 2] FAILED: log missing 'Game chat: Alice: Hi from game'" -ForegroundColor Red
        }
    } else {
        $pass = $false
        Write-Host "[Test 2] FAILED: log file not found at $logFile" -ForegroundColor Red
    }
} catch {
    $pass = $false
    Write-Host "[Test 2] FAILED: $_" -ForegroundColor Red
}

# Test 3: Verify HTTP server is NOT running
try {
    $tcp = New-Object System.Net.Sockets.TcpClient
    $tcp.Connect("127.0.0.1", 9876)
    if ($tcp.Connected) {
        $tcp.Close()
        $pass = $false
        Write-Host "[Test 3] FAILED: HTTP server is still listening on port 9876" -ForegroundColor Red
    }
} catch {
    Write-Host "[Test 3] HTTP server is not running (port 9876 closed)" -ForegroundColor Green
}

Pop-Location

Write-Host ""
if ($pass) {
    Write-Host "ALL TESTS PASSED" -ForegroundColor Green
    exit 0
} else {
    Write-Host "SOME TESTS FAILED" -ForegroundColor Red
    exit 1
}
