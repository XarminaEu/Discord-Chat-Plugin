# Discord Bot - Plugin Integration Guide

## Übersicht
Dein bestehendes Discord Bot muss erweitert werden, um Nachrichten an das Palworld-Plugin zu senden.

---

## Kommunikations-Architektur

```
Discord Channel
       ↓
   Bot (dein Projekt)
       ↓
   HTTP POST Request
       ↓
Plugin HTTP-Endpoint (Port 8765)
       ↓
Palworld Chat
```

---

## Plugin-Seite: HTTP-Endpoint

**Endpoint:** `http://localhost:8765/discord/message`

**Methode:** `POST`

**Content-Type:** `application/json`

**Request-Body:**
```json
{
  "author": "DiscordUsername",
  "content": "Deine Nachricht hier",
  "timestamp": 1718520240
}
```

**Response (Success):**
```json
{
  "status": "ok",
  "message": "Message sent to chat"
}
```

**Response (Error):**
```json
{
  "status": "error",
  "message": "Error description"
}
```

---

## Bot-Seite: Erweiterung (dein Projekt)

### Python (Discord.py)

```python
import discord
import aiohttp
from discord.ext import commands

class DiscordPalworldBridge(commands.Cog):
    def __init__(self, bot):
        self.bot = bot
        self.plugin_url = "http://localhost:8765/discord/message"
        self.channel_id = 123456789  # Dein Channel
    
    @commands.Cog.listener()
    async def on_message(self, message):
        # Ignoriere Bot-Nachrichten
        if message.author.bot:
            return
        
        # Nur im konfigurierten Channel
        if message.channel.id != self.channel_id:
            return
        
        # Sende an Plugin
        await self.send_to_palworld(message)
    
    async def send_to_palworld(self, message):
        payload = {
            "author": message.author.name,
            "content": message.content,
            "timestamp": int(message.created_at.timestamp())
        }
        
        try:
            async with aiohttp.ClientSession() as session:
                async with session.post(
                    self.plugin_url,
                    json=payload,
                    timeout=aiohttp.ClientTimeout(total=5)
                ) as resp:
                    if resp.status != 200:
                        print(f"Plugin error: {await resp.text()}")
        except Exception as e:
            print(f"Failed to send message to Palworld: {e}")

async def setup(bot):
    await bot.add_cog(DiscordPalworldBridge(bot))
```

### JavaScript (Discord.js)

```javascript
const { Client, Events, ChannelType } = require('discord.js');
const axios = require('axios');

const PLUGIN_URL = 'http://localhost:8765/discord/message';
const CHANNEL_ID = '123456789'; // Dein Channel

client.on(Events.MessageCreate, async (message) => {
    // Ignoriere Bot-Nachrichten
    if (message.author.bot) return;
    
    // Nur im konfigurierten Channel
    if (message.channelId !== CHANNEL_ID) return;
    
    // Sende an Plugin
    await sendToPalworld(message);
});

async function sendToPalworld(message) {
    const payload = {
        author: message.author.username,
        content: message.content,
        timestamp: Math.floor(message.createdTimestamp / 1000)
    };
    
    try {
        const response = await axios.post(PLUGIN_URL, payload, {
            timeout: 5000
        });
        console.log('Message sent to Palworld:', response.data);
    } catch (error) {
        console.error('Failed to send message to Palworld:', error.message);
    }
}
```

---

## Konfiguration

### Plugin-Seite (config.json)
```json
{
  "discord": {
    "webhook_url": "https://discord.com/api/webhooks/...",
    "bot_token": "NICHT NÖTIG - wird vom externen Bot genutzt"
  },
  "plugin": {
    "debug_mode": true,
    "log_file": "PalworldDiscordPlugin.log",
    "http_port": 8765,
    "http_bind": "127.0.0.1"
  }
}
```

### Bot-Seite
Konfiguriere in deinem Bot-Projekt:
- `PLUGIN_URL = "http://localhost:8765/discord/message"`
- `CHANNEL_ID = "dein-channel-id"`

---

## Sicherheit

### Authentifizierung (Optional)
Für zusätzliche Sicherheit kann ein API-Key hinzugefügt werden:

**Request mit API-Key:**
```json
{
  "api_key": "your-secret-key",
  "author": "DiscordUsername",
  "content": "Nachricht",
  "timestamp": 1718520240
}
```

**Plugin-Config:**
```json
{
  "plugin": {
    "api_key": "your-secret-key"
  }
}
```

### Firewall
- Endpoint nur auf `127.0.0.1` binden (localhost)
- Nur lokale Verbindungen erlauben
- Falls Bot auf anderem Server: IP-Whitelist nutzen

---

## Testing

### Mit cURL testen
```bash
curl -X POST http://localhost:8765/discord/message \
  -H "Content-Type: application/json" \
  -d '{"author":"TestUser","content":"Hello Palworld","timestamp":1718520240}'
```

### Mit Python testen
```python
import requests

response = requests.post(
    'http://localhost:8765/discord/message',
    json={
        'author': 'TestUser',
        'content': 'Hello Palworld',
        'timestamp': 1718520240
    }
)
print(response.json())
```

---

## Fehlerbehandlung

| Error | Ursache | Lösung |
|-------|--------|--------|
| Connection refused | Plugin nicht gestartet | Plugin starten |
| Timeout | Plugin antwortet nicht | Plugin-Logs prüfen |
| 400 Bad Request | Ungültiges JSON-Format | Payload-Format überprüfen |
| 401 Unauthorized | API-Key falsch | API-Key in Config überprüfen |

---

## Nächste Schritte

1. **Plugin entwickeln** mit HTTP-Endpoint
2. **Bot erweitern** mit obigem Code
3. **Testen** mit cURL
4. **Deployment** auf Server
