# Discord Bot - Plugin Integration Guide

> **Hinweis:** Ab Version 4.0.2 startet das Plugin **keinen HTTP-Server** mehr. Discord → Palworld läuft jetzt ausschließlich über die Datei-Bridge (`PalDiscordBridge_in.txt` / konfigurierbar in `config.json` unter `bridge.incoming_file`). Der Bot muss direkt in diese Datei schreiben (z.B. über Netzwerk-Freigabe oder wenn er auf demselben Server läuft). Die HTTP-Endpoint-Beispiele unten sind nicht mehr gültig.

## Übersicht
Dein bestehendes Discord Bot muss erweitert werden, um Nachrichten an das Palworld-Plugin zu senden.

---

## Kommunikations-Architektur

```
Discord Channel
       ↓
   Bot (dein Projekt)
       ↓
Schreibt in bridge_in.txt
       ↓
UE4SS Lua Mod liest Datei
       ↓
Palworld Chat
```

---

## Plugin-Seite: Datei-Bridge

**Eingehende Datei:** `PalDiscordBridge_in.txt` (oder konfigurierbar in `config.json` unter `bridge.incoming_file`)

**Format pro Zeile:** `discord|Autor|Nachricht`

**Beispiel:**
```
discord|TestUser|Hello Palworld
```

Das Plugin (bzw. die UE4SS Lua Bridge) liest diese Datei und gibt die Nachricht im Spiel-Chat aus.

---

## Bot-Seite: Erweiterung (dein Projekt)

### Python (Discord.py)

```python
import discord
from discord.ext import commands

class DiscordPalworldBridge(commands.Cog):
    def __init__(self, bot):
        self.bot = bot
        self.bridge_file = r"C:\Pfad\zu\Palworld\Binaries\Win64\PalDiscordBridge_in.txt"
        self.channel_id = 123456789  # Dein Channel
    
    @commands.Cog.listener()
    async def on_message(self, message):
        # Ignoriere Bot-Nachrichten
        if message.author.bot:
            return
        
        # Nur im konfigurierten Channel
        if message.channel.id != self.channel_id:
            return
        
        # Sende an Plugin über Datei-Bridge
        await self.send_to_palworld(message)
    
    async def send_to_palworld(self, message):
        line = f"discord|{message.author.name}|{message.content}\n"
        try:
            with open(self.bridge_file, "a", encoding="utf-8") as f:
                f.write(line)
        except Exception as e:
            print(f"Failed to write to Palworld bridge: {e}")

async def setup(bot):
    await bot.add_cog(DiscordPalworldBridge(bot))
```

### JavaScript (Discord.js)

```javascript
const { Client, Events, ChannelType } = require('discord.js');
const fs = require('fs');

const BRIDGE_FILE = 'C:\\Pfad\\zu\\Palworld\\Binaries\\Win64\\PalDiscordBridge_in.txt';
const CHANNEL_ID = '123456789'; // Dein Channel

client.on(Events.MessageCreate, async (message) => {
    // Ignoriere Bot-Nachrichten
    if (message.author.bot) return;
    
    // Nur im konfigurierten Channel
    if (message.channelId !== CHANNEL_ID) return;
    
    // Sende an Plugin über Datei-Bridge
    await sendToPalworld(message);
});

async function sendToPalworld(message) {
    const line = `discord|${message.author.username}|${message.content}\n`;
    try {
        fs.appendFileSync(BRIDGE_FILE, line, 'utf8');
        console.log('Message written to Palworld bridge');
    } catch (error) {
        console.error('Failed to write to Palworld bridge:', error.message);
    }
}
```

---

## Konfiguration

### Plugin-Seite (config.json)
```json
{
  "discord": {
    "webhook_url": "https://discord.com/api/webhooks/..."
  },
  "plugin": {
    "debug_mode": true,
    "log_file": "PalworldDiscordPlugin.log"
  },
  "bridge": {
    "enabled": true,
    "incoming_file": "PalDiscordBridge_in.txt"
  }
}
```

### Bot-Seite
Konfiguriere in deinem Bot-Projekt:
- `BRIDGE_FILE = "Pfad/zu/PalDiscordBridge_in.txt"`
- `CHANNEL_ID = "dein-channel-id"`

---

## Sicherheit

### Datei-Bridge
- Stelle sicher, dass nur der Discord Bot (oder vertraute Prozesse) Schreibzugriff auf die eingehende Bridge-Datei haben.
- Betreibe den Bot möglichst auf demselben Server wie Palworld, um Netzwerk-Freigaben zu vermeiden.

### API-Key
Der Plugin-Start selbst wird durch den hartcodierten API-Key und den Remote-Check bei `rl-dev.de` geschützt. Der Bot benötigt keinen eigenen API-Key für die Datei-Bridge.

---

## Testing

### Datei-Bridge testen
Schreibe eine Zeile in die eingehende Bridge-Datei:

```powershell
Add-Content -Path "PalDiscordBridge_in.txt" -Value "discord|TestUser|Hello Palworld"
```

Die Nachricht sollte kurz darauf im Spiel-Chat erscheinen.

### Mit Python testen
```python
bridge_path = r"C:\Pfad\zu\Palworld\Binaries\Win64\PalDiscordBridge_in.txt"
with open(bridge_path, "a", encoding="utf-8") as f:
    f.write("discord|TestUser|Hello Palworld\n")
```

---

## Fehlerbehandlung

| Problem | Ursache | Lösung |
|---------|--------|--------|
| Nachricht erscheint nicht im Chat | Bridge-Datei wird nicht gelesen | Pfad in `config.json` prüfen; Datei muss im Palworld-Arbeitsverzeichnis liegen |
| Berechtigungsfehler | Bot darf nicht in Datei schreiben | Schreibberechtigungen prüfen |
| Plugin startet nicht | API-Key oder Remote-Check fehlgeschlagen | Logs prüfen; `rl-dev.de/api/copyright-check` muss erreichbar sein |

---

## Nächste Schritte

1. **Plugin bauen** und in Palworld einbinden
2. **Bot erweitern**, sodass er in `bridge_in.txt` schreibt
3. **Testen** durch Schreiben in die Bridge-Datei
4. **Deployment** auf Server
