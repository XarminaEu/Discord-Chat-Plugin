# Palworld Discord Plugin - Log-Beispiel

## Server Console Output (beim Plugin-Load)

```
================================================================================
[PalworldDiscordPlugin] Loading plugin...
[PalworldDiscordPlugin] Version: 1.0.0
[PalworldDiscordPlugin] Author: Discord Integration
================================================================================
[PalworldDiscordPlugin] Config loaded successfully
[PalworldDiscordPlugin] Debug mode ENABLED
[PalworldDiscordPlugin] Initializing core systems...
[PalworldDiscordPlugin] ✓ Plugin initialized successfully
[PalworldDiscordPlugin] ✓ Chat Hook: Active
[PalworldDiscordPlugin] ✓ Discord Webhook: Configured
[PalworldDiscordPlugin] ✓ Logging: PalworldDiscordPlugin.log
================================================================================
[PalworldDiscordPlugin] Plugin loaded and ready!
================================================================================
```

## Server Console Output (beim Plugin-Unload)

```
================================================================================
[PalworldDiscordPlugin] Unloading plugin...
[PalworldDiscordPlugin] ✓ Plugin unloaded
================================================================================
```

## Plugin Log-Datei (PalworldDiscordPlugin.log)

```
[2024-06-16 07:15:23.456] [INFO] ================================================================================
[2024-06-16 07:15:23.457] [INFO] PalworldDiscordPlugin v1.0.0 - Loading
[2024-06-16 07:15:23.458] [INFO] ================================================================================
[2024-06-16 07:15:23.459] [INFO] Config loaded from config.json
[2024-06-16 07:15:23.460] [INFO] Initializing core systems...
[2024-06-16 07:15:23.461] [INFO] Plugin loaded and ready!
[2024-06-16 07:15:23.463] [INFO] Chat Hook: Active
[2024-06-16 07:15:23.464] [INFO] Discord Webhook: Configured
[2024-06-16 07:15:23.465] [INFO] ================================================================================
[2024-06-16 07:15:45.123] [DEBUG] Broadcasting message: PlayerName: Hello World
[2024-06-16 07:15:45.124] [INFO] Message sent to Discord webhook
[2024-06-16 07:16:12.456] [DEBUG] Bridge file read: discord|TestBot|Hello from Discord
[2024-06-16 07:16:12.458] [INFO] Broadcasting Discord message to game chat
[2024-06-16 07:16:30.789] [WARNING] Failed to send message to Discord webhook
[2024-06-16 07:16:30.790] [ERROR] CURL error: Connection timeout
[2024-06-16 07:20:15.234] [INFO] ================================================================================
[2024-06-16 07:20:15.235] [INFO] PalworldDiscordPlugin - Shutting down
[2024-06-16 07:20:15.236] [INFO] Plugin unloaded successfully
[2024-06-16 07:20:15.237] [INFO] ================================================================================
```

## Fehler-Beispiele

### Config-Datei nicht gefunden
```
[PalworldDiscordPlugin] WARNING: config.json not found, using defaults
[2024-06-16 07:15:23.459] [WARNING] Failed to load config.json, using defaults
```

### Plugin-Initialisierung fehlgeschlagen
```
[PalworldDiscordPlugin] ERROR: Failed to initialize plugin!
================================================================================
[2024-06-16 07:15:23.461] [CRITICAL] Failed to initialize plugin
```

### Discord Webhook nicht konfiguriert
```
[PalworldDiscordPlugin] ✓ Discord Webhook: Not configured
```

---

## Log-Level Erklärung

| Level | Farbe | Bedeutung | Beispiel |
|-------|-------|-----------|----------|
| **DEBUG** | Grau | Detaillierte Debug-Informationen | Nachricht gesendet, HTTP-Request erhalten |
| **INFO** | Weiß | Allgemeine Informationen | Plugin geladen, Config geladen |
| **WARNING** | Gelb | Warnungen (nicht kritisch) | Config nicht gefunden, Retry-Versuch |
| **ERROR** | Rot | Fehler (aber Plugin läuft noch) | Webhook-Fehler, Timeout |
| **CRITICAL** | Rot (Fett) | Kritische Fehler (Plugin-Crash) | Plugin-Initialisierung fehlgeschlagen |

---

## Wo findest du die Logs?

1. **Server Console** - Beim Starten des Servers (stdout/stderr)
2. **PalworldDiscordPlugin.log** - Im Verzeichnis der DLL
   - Standardmäßig: `Palworld/Binaries/Win64/PalworldDiscordPlugin.log`

---

## Debug-Modus aktivieren

In `config.json`:
```json
{
  "plugin": {
    "debug_mode": true
  }
}
```

Damit werden auch DEBUG-Nachrichten in der Konsole angezeigt.

---

## Tipps zum Debugging

1. **Starte den Server mit Konsole-Fenster**
   - So kannst du die Live-Logs sehen

2. **Aktiviere Debug-Modus**
   - Mehr Details in den Logs

3. **Überprüfe die Log-Datei**
   - Auch nach Server-Crash verfügbar

4. **Nutze grep/findstr zum Filtern**
   ```powershell
   Get-Content PalworldDiscordPlugin.log | Select-String "ERROR"
   ```
