# Palworld Discord Chat Plugin

A native C++ DLL server plugin for **Palworld** that bridges the in-game chat with **Discord**.

> **Non-commercial version** — free for personal and community servers only.  
> **Copyright 2026 RL-Dev.de** — see [LICENSE.md](LICENSE.md) for terms.

## Features

- **Game Chat → Discord**: Player messages are forwarded to a Discord webhook.
- **Discord → Game Chat**: Discord messages are injected into the Palworld chat via the file bridge.
- **Webhook Integration**: Sends rich embeds for chat, join, leave and death events.
- **UE4SS Lua Bridge**: Ships with a Lua mod for in-game chat hooking without recompiling UE4SS.
- **Logging**: Detailed logging to console and `PalworldDiscordPlugin.log`.
- **Copyright & License Protection**: Hardcoded, obfuscated copyright and license values; the plugin refuses to start if protected values are modified.

## Requirements

- Windows 10/11
- Visual Studio Build Tools 2022 (C++ compiler)
- CMake 3.20+
- Git
- Optional: [vcpkg](https://github.com/microsoft/vcpkg) for dependencies

## Quick Start

### 1. Install dependencies

```powershell
# Visual Studio Build Tools
https://visualstudio.microsoft.com/visual-cpp-build-tools/

# CMake
https://cmake.org/download/

# vcpkg
git clone https://github.com/Microsoft/vcpkg.git C:\dev\vcpkg
cd C:\dev\vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

### 2. Install required libraries

```powershell
cd C:\dev\vcpkg
.\vcpkg install curl:x64-windows
.\vcpkg install nlohmann-json:x64-windows
.\vcpkg install sqlite3:x64-windows
```

### 3. Build the plugin

```powershell
cd D:\Palworld\PalworldDiscordPlugin
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\dev\vcpkg\scripts\buildsystems\vcpkg.cmake -G "Visual Studio 17 2022"
cmake --build . --config Release
```

The DLL is written to `build/bin/PalworldDiscordPlugin.dll`.

### 4. Deploy

```powershell
copy D:\Palworld\PalworldDiscordPlugin\build\bin\PalworldDiscordPlugin.dll "C:\Program Files\Palworld\Binaries\Win64\"
copy D:\Palworld\PalworldDiscordPlugin\config\config.json "C:\Program Files\Palworld\Binaries\Win64\"
```

## Configuration

Edit `config.json` next to the DLL:

```json
{
  "discord": {
    "webhook_url": "https://discord.com/api/webhooks/YOUR_ID/YOUR_TOKEN",
    "bot_token": "YOUR_BOT_TOKEN",
    "server_id": "YOUR_SERVER_ID",
    "channel_id": "YOUR_CHANNEL_ID"
  },
  "plugin": {
    "debug_mode": true,
    "log_file": "PalworldDiscordPlugin.log",
    "max_message_length": 2000
  },
  "ue4ss": {
    "chat_recv_function": "",
    "chat_message_param": "",
    "chat_sender_param": "",
    "chat_send_function": ""
  },
  "bridge": {
    "enabled": true,
    "incoming_file": "PalDiscordBridge_in.txt",
    "outgoing_file": "PalDiscordBridge_out.txt"
  }
}
```

> ⚠️ **Do not edit any values related to the plugin license, copyright or product identity.** The plugin validates these against encrypted hardcoded values at startup and will refuse to load if they do not match.

## Remote Copyright Check

At startup the plugin sends a `POST` request to the external service:

```
https://rl-dev.de/api/copyright-check
```

Payload:

```json
{
  "program": "PalDiscordPlugin",
  "copyright": "Copyright 2026 RL-Dev.de"
}
```

The server validates the license and responds with:

```json
{ "status": "ok", "allowed": true, "message": "startet" }
```

If the license is blocked:

```json
{ "reason": "gesperrt" }
```

On any other response the plugin refuses to start. The server side (blocklist, IP logging, "Sperren" button) is managed by `rl-dev.de`.

## Discord Bot Setup

See [BOT_INTEGRATION.md](BOT_INTEGRATION.md) for Python (`discord.py`) and JavaScript (`discord.js`) examples.

## UE4SS Integration

See [UE4SS_INTEGRATION.md](UE4SS_INTEGRATION.md) for the recommended Lua bridge setup and the optional C++ mod build.

## Project Structure

```
PalworldDiscordPlugin/
├── src/                    # C++ source code
├── include/                # C++ headers
├── config/                 # Example config.json
├── ue4ss_lua/              # UE4SS Lua bridge mod
├── test/                   # Test scripts
├── build/                  # Build output
├── CMakeLists.txt          # CMake configuration
├── README.md               # This file
├── LICENSE.md              # License terms
└── tools/                  # Helper scripts
```

## Security Notes

- Keep your Discord webhook URL and bot token secret.
- Do not edit or remove any copyright, product name or license-related values.
- Copyright and product name strings are XOR-obfuscated in the source and binary.
- The generator script that produced the encrypted byte arrays is not part of the public repository.
- The obfuscation raises the bar for casual tampering but cannot stop a determined reverse engineer.

## Logging

Logs are written to the file configured in `config.json` (default `PalworldDiscordPlugin.log`).

Enable debug mode for more verbose output:

```json
"debug_mode": true
```

## License

Copyright 2026 RL-Dev.de.  
See [LICENSE.md](LICENSE.md) for the full non-commercial license.

## Support

- [SETUP.md](SETUP.md) — build and setup instructions
- [BOT_INTEGRATION.md](BOT_INTEGRATION.md) — Discord bot integration
- [UE4SS_INTEGRATION.md](UE4SS_INTEGRATION.md) — in-game chat hooking
- [LOG_EXAMPLE.md](LOG_EXAMPLE.md) — example log output
