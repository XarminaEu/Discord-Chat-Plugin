# PalworldDiscordPlugin - Auto-Deploy
# Legt alles an die richtigen Stellen. Kein manuelles Kopieren mehr.
# Beispiel-Aufruf:
#   .\deploy.ps1 -PalworldPath "D:\Games\Steam\steamapps\common\Palworld\Pal\Binaries\Win64"
#   .\deploy.ps1 -PalworldPath "C:\PalServer\steamapps\common\PalServer\Pal\Binaries\Win64"

param(
    [Parameter(Mandatory=$true, HelpMessage="Pfad zu Pal/Binaries/Win64")]
    [string]$PalworldPath
)

$ErrorActionPreference = "Stop"

# Pfade aufbauen
$Win64       = $PalworldPath.TrimEnd('\','/')
$Ue4ssMods   = Join-Path $Win64 "ue4ss\Mods"
$ModDir      = Join-Path $Ue4ssMods "PalworldDiscordBridge"
$ModScripts  = Join-Path $ModDir "Scripts"
$BuildDir    = Join-Path $PSScriptRoot "build\bin\Release"
$LuaSrc      = Join-Path $PSScriptRoot "ue4ss_lua"

# Pruefen
if (-not (Test-Path $Win64)) {
    Write-Host "[FEHLER] Pfad existiert nicht: $Win64" -ForegroundColor Red
    exit 1
}
if (-not (Test-Path (Join-Path $Win64 "ue4ss"))) {
    Write-Host "[FEHLER] Kein ue4ss-Ordner gefunden. Stimmt der Pfad?" -ForegroundColor Red
    Write-Host "         Erwartet: ...\Pal\Binaries\Win64" -ForegroundColor Yellow
    exit 1
}
if (-not (Test-Path (Join-Path $BuildDir "PalworldDiscordPlugin.dll"))) {
    Write-Host "[FEHLER] DLL nicht gefunden. Bitte zuerst bauen:" -ForegroundColor Red
    Write-Host "         cmake --build build --config Release" -ForegroundColor Yellow
    exit 1
}

Write-Host "[*] Deploye PalworldDiscordPlugin nach:" -ForegroundColor Cyan
Write-Host "    $Win64" -ForegroundColor Gray

# 1. DLL direkt nach Win64 kopieren (wie PalDefender)
$DllSource = Join-Path $BuildDir "PalworldDiscordPlugin.dll"
$DllTarget = Join-Path $Win64 "PalworldDiscordPlugin.dll"
Copy-Item -Path $DllSource -Destination $DllTarget -Force
Write-Host "[OK] DLL -> $DllTarget" -ForegroundColor Green

# 2. Lua-Mod-Ordner anlegen
New-Item -ItemType Directory -Force -Path $ModScripts | Out-Null

# 3. Lua-Skript anpassen (sucht DLL in Win64 statt im Mod-Ordner)
$LuaContent = Get-Content (Join-Path $LuaSrc "Scripts\main.lua") -Raw
# Ersetze dll_path, damit es direkt Win64 sucht
$LuaContent = $LuaContent -replace 
    'dll_path = "[^"]+"',
    'dll_path = "PalworldDiscordPlugin.dll"'
Set-Content -Path (Join-Path $ModScripts "main.lua") -Value $LuaContent -Encoding UTF8
Write-Host "[OK] Lua-Skript -> $(Join-Path $ModScripts 'main.lua')" -ForegroundColor Green

# 4. mod.txt
Copy-Item -Path (Join-Path $LuaSrc "mod.txt") -Destination (Join-Path $ModDir "mod.txt") -Force
Write-Host "[OK] mod.txt" -ForegroundColor Green

# 5. Mod in mods.txt aktivieren
$ModsTxt = Join-Path $Ue4ssMods "mods.txt"
$ModEntry = "PalworldDiscordBridge : 1"
if (Test-Path $ModsTxt) {
    $content = Get-Content $ModsTxt -Raw
    if ($content -notmatch [regex]::Escape($ModEntry)) {
        Add-Content -Path $ModsTxt -Value $ModEntry -Encoding UTF8
        Write-Host "[OK] mods.txt aktualisiert ($ModEntry)" -ForegroundColor Green
    } else {
        Write-Host "[OK] mods.txt enthielt Eintrag bereits" -ForegroundColor Green
    }
} else {
    Set-Content -Path $ModsTxt -Value $ModEntry -Encoding UTF8
    Write-Host "[OK] mods.txt erstellt" -ForegroundColor Green
}

# 6. config.json Beispiel erstellen (falls noch nicht vorhanden)
$ConfigPath = Join-Path $Win64 "config.json"
if (-not (Test-Path $ConfigPath)) {
    $ConfigJson = @'{
  "discord": {
    "webhook_url": "https://discord.com/api/webhooks/DEIN_WEBHOK_HIER"
  },
  "plugin": {
    "debug_mode": true,
    "log_file": "PalworldDiscordPlugin.log",
    "http_port": 8765,
    "http_bind": "127.0.0.1",
    "api_key": ""
  },
  "bridge": {
    "enabled": true,
    "incoming_file": "PalDiscordBridge_in.txt",
    "outgoing_file": "PalDiscordBridge_out.txt"
  }
}
'@
    Set-Content -Path $ConfigPath -Value $ConfigJson -Encoding UTF8
    Write-Host "[OK] config.json erstellt (bitte Webhook-URL eintragen!)" -ForegroundColor Yellow
} else {
    Write-Host "[OK] config.json existierte bereits" -ForegroundColor Green
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "DEPLOYMENT ABGESCHLOSSEN" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Naechste Schritte:" -ForegroundColor White
Write-Host "  1. Oeffne $ConfigPath" -ForegroundColor Gray
Write-Host "     -> Trage deine Discord-Webhook-URL ein." -ForegroundColor Gray
Write-Host "  2. Starte Palworld/Server neu." -ForegroundColor Gray
Write-Host "  3. Im Spiel chatten -> Discord-Webhook sollte ankommen." -ForegroundColor Gray
Write-Host ""
