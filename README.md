# Palworld Discord Chat Plugin

A native C++ DLL server plugin for **Palworld** that bridges the in-game chat with **Discord**.

> **Non-commercial version** — free for personal and community servers only.  
> **Copyright 2026 RL-Dev.de** — see [LICENSE.md](LICENSE.md) for terms.

## Features

- **Game Chat → Discord**: Player messages are forwarded to a Discord webhook.
- **Discord → Game Chat**: Discord messages are injected into the Palworld chat via the file bridge.
- **Webhook Integration**: Sends rich embeds for chat, join, leave and death events.
- **UE4SS Lua Bridge**: Ships with a Lua mod for in-game chat hooking without recompiling UE4SS.

## Installation

1. Download the latest release zip from the [Releases](../../releases) page or use the `PalworldDiscordPlugin_v4.0.5.zip` in this repository.
2. Stop your Palworld server.
3. Extract the ZIP directly into your server's `Pal/Binaries/Win64/` folder.

   Expected structure after extraction:
   ```
   Win64/
   ├── PalworldDiscordPlugin.dll
   ├── PalworldDiscordConfig/
   │   └── config.json
   └── ue4ss/
       └── Mods/
           └── PalworldDiscordBridge/
               ├── mod.txt
               ├── Scripts/
               │   └── main.lua
               └── dlls/           (empty folder, used for the file bridge)
   ```
4. Open `Win64/ue4ss/Mods/mods.txt` and add the line:
   ```
   PalworldDiscordBridge : 1
   ```
5. Open `Win64/PalworldDiscordConfig/config.json` and enter your Discord webhook URL (and bot token if you want Discord → Game chat).
6. Start the server.

The first start writes the remaining bridge files automatically.

## Configuration

Edit `PalworldDiscordConfig/config.json` next to the DLL:

```json
{
  "discord": {
    "webhook_url": "https://discord.com/api/webhooks/YOUR_ID/YOUR_TOKEN",
    "bot_token": "YOUR_BOT_TOKEN",
    "server_id": "YOUR_SERVER_ID",
    "channel_id": "YOUR_CHANNEL_ID",
    "bot_name": "Server Chat",
    "language": "de",
    "discord_to_game": true,
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
    "incoming_file": "ue4ss/Mods/PalworldDiscordBridge/dlls/PalDiscordBridge_in.txt",
    "outgoing_file": "ue4ss/Mods/PalworldDiscordBridge/dlls/PalDiscordBridge_out.txt"
  }
}
```

### Important settings

- `discord.webhook_url`: Required for **Game Chat → Discord**. Create a webhook in your Discord server and paste the URL here.
- `discord.bot_token` + `discord.channel_id`: Required for **Discord → Game Chat**. The bot needs `Read Message History` permission in the channel.
- `events`: Enable or disable individual event types.
- `bridge`: Leave the default paths as shown above. The Lua mod and the C++ plugin must read/write the same files.

> ⚠️ **Do not edit `api_key`, product name, copyright or any other license-related values.** The plugin validates them at startup and will refuse to load if they do not match.

## Troubleshooting

- Check `PalworldDiscordPlugin.log` in `Win64/` for errors.
- Make sure `PalworldDiscordBridge : 1` is in `ue4ss/Mods/mods.txt`.
- Make sure the `ue4ss/Mods/PalworldDiscordBridge/dlls/` folder exists.
- Enable `debug_mode: true` for more detailed logs.

## Discord Bot Setup

See [BOT_INTEGRATION.md](BOT_INTEGRATION.md) for Python (`discord.py`) and JavaScript (`discord.js`) examples.

## License

Copyright 2026 RL-Dev.de.  
See [LICENSE.md](LICENSE.md) for the full non-commercial license.
