# Runtime test for PalworldDiscordPlugin
# Loads the DLL (which starts the HTTP server) and POSTs a test message to the endpoint.

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$dll = Join-Path $root "build\bin\Release\PalworldDiscordPlugin.dll"
$testDir = $PSScriptRoot

if (-not (Test-Path $dll)) {
    Write-Host "ERROR: DLL not found at $dll" -ForegroundColor Red
    exit 1
}

# Place config.json in the test working directory (DllMain loads it from CWD)
Copy-Item (Join-Path $testDir "test_config.json") (Join-Path $testDir "config.json") -Force

# Clean any leftover bridge files from a previous run
Remove-Item (Join-Path $testDir "bridge_in.txt") -ErrorAction SilentlyContinue
Remove-Item (Join-Path $testDir "bridge_out.txt") -ErrorAction SilentlyContinue
Remove-Item (Join-Path $testDir "PalworldDiscordPlugin_test.log") -ErrorAction SilentlyContinue

Push-Location $testDir
[System.Environment]::CurrentDirectory = $testDir

# P/Invoke LoadLibrary / FreeLibrary
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Native {
    [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Ansi)]
    public static extern IntPtr LoadLibrary(string path);
    [DllImport("kernel32.dll", SetLastError=true)]
    public static extern bool FreeLibrary(IntPtr hModule);
}
"@

Write-Host "[*] Loading plugin DLL..." -ForegroundColor Cyan
$h = [Native]::LoadLibrary($dll)
if ($h -eq [IntPtr]::Zero) {
    Write-Host "ERROR: LoadLibrary failed (error $([System.Runtime.InteropServices.Marshal]::GetLastWin32Error()))" -ForegroundColor Red
    Pop-Location
    exit 1
}
Write-Host "[*] DLL loaded (handle $h). Waiting for HTTP server..." -ForegroundColor Green
Start-Sleep -Seconds 2

# Bypass any system proxy for localhost requests
[System.Net.WebRequest]::DefaultWebProxy = $null

$pass = $true

# Test 1: valid message
try {
    $body = '{"author":"Tester","content":"Hello from Discord"}'
    $resp = Invoke-RestMethod -Uri "http://127.0.0.1:9876/discord/message" -Method Post -Body $body -ContentType "application/json" -TimeoutSec 5
    Write-Host "[Test 1] Valid message -> $($resp | ConvertTo-Json -Compress)" -ForegroundColor Green
    if ($resp.status -ne "ok") { $pass = $false; Write-Host "  FAILED: expected status ok" -ForegroundColor Red }
} catch {
    $pass = $false
    Write-Host "[Test 1] FAILED: $_" -ForegroundColor Red
}

# Test 2: missing content
try {
    $body = '{"author":"Tester"}'
    $resp = Invoke-RestMethod -Uri "http://127.0.0.1:9876/discord/message" -Method Post -Body $body -ContentType "application/json" -TimeoutSec 5
    Write-Host "[Test 2] Missing content -> $($resp | ConvertTo-Json -Compress)" -ForegroundColor Green
    if ($resp.status -ne "error") { $pass = $false; Write-Host "  FAILED: expected status error" -ForegroundColor Red }
} catch {
    $pass = $false
    Write-Host "[Test 2] FAILED: $_" -ForegroundColor Red
}

# Test 3: invalid JSON
try {
    $body = 'not json at all'
    $resp = Invoke-RestMethod -Uri "http://127.0.0.1:9876/discord/message" -Method Post -Body $body -ContentType "application/json" -TimeoutSec 5
    Write-Host "[Test 3] Invalid JSON -> $($resp | ConvertTo-Json -Compress)" -ForegroundColor Green
    if ($resp.status -ne "error") { $pass = $false; Write-Host "  FAILED: expected status error" -ForegroundColor Red }
} catch {
    $pass = $false
    Write-Host "[Test 3] FAILED: $_" -ForegroundColor Red
}

# Test 4: Discord -> game wrote to the incoming bridge file (from Test 1)
try {
    Start-Sleep -Milliseconds 300
    $inFile = Join-Path $testDir "bridge_in.txt"
    $content = if (Test-Path $inFile) { Get-Content $inFile -Raw } else { "" }
    if ($content -match "Tester`tHello from Discord") {
        Write-Host "[Test 4] Bridge incoming file written correctly" -ForegroundColor Green
    } else {
        $pass = $false
        Write-Host "[Test 4] FAILED: bridge_in.txt missing expected line. Got: '$content'" -ForegroundColor Red
    }
} catch {
    $pass = $false
    Write-Host "[Test 4] FAILED: $_" -ForegroundColor Red
}

# Test 5: game -> Discord: writing to outgoing bridge file triggers OnGameChatMessage
try {
    $outFile = Join-Path $testDir "bridge_out.txt"
    "Alice`tHi from game" | Out-File -FilePath $outFile -Append -Encoding ascii
    # Wait for FileBridge watcher poll (300ms interval) + processing
    Start-Sleep -Milliseconds 800
    # The log line "[DEBUG] Game chat: Alice: Hi from game" should be visible.
    # We verify it after DLL unload when the log file handle is closed.
    $gameChatFound = $true
} catch {
    $pass = $false
    Write-Host "[Test 5] FAILED: $_" -ForegroundColor Red
}

Write-Host "[*] Unloading DLL..." -ForegroundColor Cyan
[Native]::FreeLibrary($h) | Out-Null

# Now the logger has closed the file; verify Test 5 log line.
try {
    $logFile = Join-Path $testDir "PalworldDiscordPlugin_test.log"
    if (Test-Path $logFile) {
        $log = Get-Content $logFile -Raw
        if ($log -match "Game chat: Alice: Hi from game") {
            Write-Host "[Test 5] Outgoing bridge line forwarded to game-chat handler" -ForegroundColor Green
        } else {
            $pass = $false
            Write-Host "[Test 5] FAILED: log missing 'Game chat: Alice: Hi from game'" -ForegroundColor Red
        }
    } else {
        $pass = $false
        Write-Host "[Test 5] FAILED: log file not found at $logFile" -ForegroundColor Red
    }
} catch {
    $pass = $false
    Write-Host "[Test 5] FAILED: $_" -ForegroundColor Red
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
