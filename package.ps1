# PalworldDiscordPlugin - Release Package Builder
# Erstellt PalworldDiscordPlugin_v$VERSION.zip fuer Distribution
# Aufruf: .\package.ps1

$ErrorActionPreference = "Stop"
$VERSION = "4.0.3"
$ROOT     = $PSScriptRoot
$BUILD    = Join-Path $ROOT "build\bin\Release"
$STAGE    = Join-Path $ROOT "build\Package_v$VERSION"
$ZIP_OUT  = Join-Path $ROOT "PalworldDiscordPlugin_v$VERSION.zip"

# ── Voraussetzungen prüfen ────────────────────────────────────────────────────
$dll = Join-Path $BUILD "PalworldDiscordPlugin.dll"
if (-not (Test-Path $dll)) {
    Write-Host "[FEHLER] DLL nicht gefunden. Zuerst bauen:" -ForegroundColor Red
    Write-Host "         cmake --build build --config Release" -ForegroundColor Yellow
    exit 1
}

Write-Host ""
Write-Host "================================================================================" -ForegroundColor Cyan
Write-Host "  PalworldDiscordPlugin v$VERSION  - Package Builder" -ForegroundColor Cyan
Write-Host "================================================================================" -ForegroundColor Cyan
Write-Host ""

# ── Staging-Ordner leeren ────────────────────────────────────────────────────
if (Test-Path $STAGE) {
    Remove-Item $STAGE -Recurse -Force
}
New-Item -ItemType Directory -Path $STAGE | Out-Null
Write-Host "[*] Staging-Ordner: $STAGE" -ForegroundColor Gray

# ── 1. DLL ───────────────────────────────────────────────────────────────────
Copy-Item $dll (Join-Path $STAGE "PalworldDiscordPlugin.dll") -Force
Write-Host "[OK] PalworldDiscordPlugin.dll" -ForegroundColor Green

# ── 2. PalworldDiscordConfig\config.json ─────────────────────────────────────
$cfgDir = Join-Path $STAGE "PalworldDiscordConfig"
New-Item -ItemType Directory -Path $cfgDir | Out-Null

$cfgContent = @'
{
  "discord": {
    "webhook_url": "",
    "bot_token": "",
    "channel_id": "",
    "bot_name": "Server Chat",
    "language": "de",
    "discord_to_game": false,
    "poll_interval": 5
  },
  "plugin": {
    "debug_mode": false,
    "log_file": "PalworldDiscordPlugin.log",
    "max_message_length": 2000,
    "api_key": "b75c2541e6eb17db69d6cf441827e19b91c13a7444eb39e89e9b7dc635b4c717"
  },
  "events": {
    "chat": true,
    "join": true,
    "leave": true,
    "death": true
  },
  "bridge": {
    "enabled": true,
    "incoming_file": "PalDiscordBridge_in.txt",
    "outgoing_file": "PalDiscordBridge_out.txt"
  }
}
'@

$cfgPath = Join-Path $cfgDir "config.json"
[System.IO.File]::WriteAllText($cfgPath, $cfgContent, [System.Text.UTF8Encoding]::new($false))
Write-Host "[OK] PalworldDiscordConfig\config.json" -ForegroundColor Green

# ── 3. UE4SS Lua-Mod ─────────────────────────────────────────────────────────
$luaSrc = Join-Path $BUILD "ue4ss\Mods\PalworldDiscordBridge"
if (-not (Test-Path $luaSrc)) {
    Write-Host "[FEHLER] Lua-Mod-Ordner nicht gefunden: $luaSrc" -ForegroundColor Red
    Write-Host "         Hinweis: Fuehre zuerst test\test_remote_check.ps1 aus" -ForegroundColor Yellow
    Write-Host "         (SelfInstall schreibt die Lua-Dateien beim ersten Start)" -ForegroundColor Yellow
    exit 1
}
$modDest = Join-Path $STAGE "ue4ss\Mods\PalworldDiscordBridge"
Copy-Item $luaSrc $modDest -Recurse -Force
# dlls-Unterordner nicht mitpacken (nur fuer den Mods-internen DLL-Pfad-Modus)
$dllsDir = Join-Path $modDest "dlls"
if (Test-Path $dllsDir) { Remove-Item $dllsDir -Recurse -Force }
Write-Host "[OK] ue4ss\Mods\PalworldDiscordBridge\" -ForegroundColor Green

# ── 4. README.txt ─────────────────────────────────────────────────────────────
$readmeTxt = @"
PalworldDiscordPlugin v$VERSION
================================================

INSTALLATION (3 Schritte)
--------------------------

SCHRITT 1: Dateien entpacken
  Entpacke den Inhalt dieses ZIPs in deinen Ordner:
    Pal\Binaries\Win64\

  Ergebnis:
    Win64\PalworldDiscordPlugin.dll
    Win64\PalworldDiscordConfig\config.json
    Win64\ue4ss\Mods\PalworldDiscordBridge\


SCHRITT 2: Mod aktivieren
  Oeffne:  Win64\ue4ss\Mods\mods.txt
  Fuege am Ende ein:

    PalworldDiscordBridge : 1

  Speichern und schliessen.


SCHRITT 3: Webhook eintragen
  Oeffne:  Win64\PalworldDiscordConfig\config.json
  Trage deine Discord-Webhook-URL ein:

    "webhook_url": "https://discord.com/api/webhooks/DEINE_URL_HIER"

  Speichern.


SCHRITT 4: Server starten
  Starte den Palworld-Server. Fertig.
  Der erste Start erstellt alle fehlenden Dateien automatisch.


FEHLERBEHEBUNG
--------------
  Log:      Win64\PalworldDiscordPlugin.log
  Pruefen:  mods.txt enthaelt "PalworldDiscordBridge : 1"
  Pruefen:  PalworldDiscordConfig\config.json enthaelt korrekte webhook_url


FEATURES
--------
  - Chat (Spiel -> Discord)
  - Beitritt / Verlassen / Tod als Discord-Embed
  - Discord -> Spiel (bot_token + channel_id konfigurieren)
  - Alle Ereignisse einzeln aktivierbar (events in config.json)
  - Automatische Selbst-Installation beim Start


VERSION
-------
  v$VERSION - PalworldDiscordConfig Edition
  Copyright 2026 RL-Dev.de
"@
[System.IO.File]::WriteAllText((Join-Path $STAGE "README.txt"), $readmeTxt, [System.Text.UTF8Encoding]::new($false))
Write-Host "[OK] README.txt" -ForegroundColor Green

# ── 5. ZIP erstellen ──────────────────────────────────────────────────────────
if (Test-Path $ZIP_OUT) { Remove-Item $ZIP_OUT -Force }
Compress-Archive -Path (Join-Path $STAGE "*") -DestinationPath $ZIP_OUT -CompressionLevel Optimal
Write-Host "[OK] ZIP erstellt: $ZIP_OUT" -ForegroundColor Green

# ── Ergebnis ──────────────────────────────────────────────────────────────────
$zipSize = [math]::Round((Get-Item $ZIP_OUT).Length / 1KB, 1)
Write-Host ""
Write-Host "================================================================================" -ForegroundColor Green
Write-Host "  Release fertig!  ->  PalworldDiscordPlugin_v$VERSION.zip  ($zipSize KB)" -ForegroundColor Green
Write-Host "================================================================================" -ForegroundColor Green
Write-Host ""
Write-Host "Inhalt:" -ForegroundColor Cyan
Get-ChildItem $STAGE -Recurse | Where-Object { -not $_.PSIsContainer } | ForEach-Object {
    Write-Host ("  " + $_.FullName.Replace($STAGE + "\", "")) -ForegroundColor Gray
}
Write-Host ""
