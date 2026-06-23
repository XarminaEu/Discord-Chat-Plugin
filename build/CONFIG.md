# PalworldDiscordPlugin - Konfigurations-Guide

## config.json

Diese Datei wird automatisch beim ersten Start erstellt. Sie liegt im Verzeichnis:
`Pal/Binaries/Win64/ue4ss/Mods/PalworldDiscordBridge/dlls/config.json`

---

### discord

**webhook_url** (string, erforderlich)
- Die Discord Webhook URL. Siehe: Servereinstellungen > Integrationen > Webhooks > URL kopieren
- Beispiel: `"https://discord.com/api/webhooks/.../..."`

**bot_name** (string, optional, Default: `"Server Chat"`)
- Der Name, unter dem der Bot in Discord erscheint
- Beispiel: `"Palworld Server"`, `"[GER] Lunaris Chat"`

**language** (string, optional, Default: `"en"`)
- Sprache der Discord-Nachrichten
- `"en"` = Englisch
- `"de"` = Deutsch

**bot_token** (string, optional, Default: `""`)
- Discord Bot Token für Zwei-Wege-Chat
- Erstelle einen Bot im [Discord Developer Portal](https://discord.com/developers/applications)
- Kopiere den Token und füge ihn hier ein
- Nur benötigt wenn `discord_to_game: true`

**channel_id** (string, optional, Default: `""`)
- Discord Channel ID aus dem Nachrichten gelesen werden
- Rechtsklick auf Channel -> "Channel-ID kopieren" (Developer Mode aktivieren)
- Nur benötigt wenn `discord_to_game: true`

**discord_to_game** (bool, optional, Default: `false`)
- `true` = Discord Nachrichten werden ins Spiel geschickt
- `false` = Nur Spiel -> Discord (einweg)

**poll_interval** (int, optional, Default: `5`)
- Sekunden zwischen Discord API Abfragen
- Minimum: 1 Sekunde

---

### plugin

**debug_mode** (bool, Default: `true`)
- `true` = ausführliche Logs in `PalworldDiscordPlugin.log`
- `false` = nur Fehler und wichtige Events loggen

**log_file** (string, Default: `"PalworldDiscordPlugin.log"`)
- Pfad zur Logdatei (relativ zum Spielverzeichnis)

**http_port** (int, Default: `8765`)
- Port des internen HTTP-Servers (für zukünftige Features)
- `0` = HTTP-Server deaktivieren

**http_bind** (string, Default: `"127.0.0.1"`)
- IP-Adresse an die der HTTP-Server bindet
- `"127.0.0.1"` = nur lokal erreichbar
- `"0.0.0.0"` = von außen erreichbar (unsicher!)

**max_message_length** (int, Default: `2000`)
- Maximale Länge von Discord-Nachrichten (Discord-Limit: 2000)
- Längere Nachrichten werden abgeschnitten

**api_key** (string, Default: `""`)
- API-Key für den HTTP-Server (zukünftiges Feature)
- Leer = keine Authentifizierung

---

### events

**chat** (bool, Default: `true`)
- `true` = Spieler-Chat wird an Discord gesendet
- `false` = Chat-Events ignorieren

**join** (bool, Default: `true`)
- `true` = "Player Join" Embeds an Discord
- `false` = Join-Events ignorieren
- **Hinweis:** Join-Hook ist derzeit deaktiviert (verursacht Disconnects)

**leave** (bool, Default: `true`)
- `true` = "Player Leave" Embeds an Discord
- `false` = Leave-Events ignorieren

**death** (bool, Default: `true`)
- `true` = "Player Death" Embeds an Discord
- `false` = Death-Events ignorieren

---

### bridge

**enabled** (bool, Default: `true`)
- `true` = Datei-Bridge zwischen Lua und C++ aktiv
- `false` = Nur C++ LogWatcher/ConsoleReader (weniger zuverlässig)

**incoming_file** (string, Default: `"PalDiscordBridge_in.txt"`)
- Datei für Discord -> Spiel Nachrichten (zukünftig)

**outgoing_file** (string, Default: `"PalDiscordBridge_out.txt"`)
- Datei für Spiel -> Discord Nachrichten
- Lua schreibt hier rein, C++ liest und sendet an Discord

---

## Beispiel: Minimal-Konfiguration

```json
{
  "discord": {
    "webhook_url": "DEINE_WEBHOOK_URL_HIER",
    "bot_name": "Palworld Chat"
  },
  "events": {
    "chat": true,
    "join": false,
    "leave": false,
    "death": false
  }
}
```

## Beispiel: Nur Chat, kein Join/Leave/Death

```json
{
  "discord": {
    "webhook_url": "DEINE_WEBHOOK_URL_HIER",
    "bot_name": "Server Chat",
    "language": "de"
  },
  "events": {
    "chat": true,
    "join": false,
    "leave": false,
    "death": false
  }
}
```

## Beispiel: Alles aktiv, Debug aus

```json
{
  "discord": {
    "webhook_url": "DEINE_WEBHOOK_URL_HIER",
    "bot_name": "Palworld Server",
    "language": "en"
  },
  "plugin": {
    "debug_mode": false,
    "log_file": "PalworldDiscordPlugin.log"
  },
  "events": {
    "chat": true,
    "join": true,
    "leave": true,
    "death": true
  }
}
```

---

## WICHTIGE HINWEISE

1. **Webhook URL:** Die URL muss mit `https://discord.com/api/webhooks/` beginnen
2. **bot_name:** Ändere dies, um den Bot-Namen in Discord anzupassen
3. **Events:** Setze auf `false`, um bestimmte Event-Typen zu deaktivieren
4. **config.json ändern:** Nach Änderungen Server NEU STARTEN
5. **Debug-Modus:** Lasse `debug_mode: true` für Fehlersuche aktiviert
