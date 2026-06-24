# Changelog

## v4.0.3 (2026-06-24)

### Bug Fixes
- **License check not triggered** - `SelfInstall` was unconditionally overwriting `config.json` on every startup, resetting `api_key` to an empty string. The local API key check then failed before the remote license check at `https://rl-dev.de/api/copyright-check` was ever reached. Fixed: `config.json` is now only created when it does not already exist; existing user configuration is preserved.
- **Fresh install missing api_key** - When creating `config.json` for the first time, the correct `api_key` is now injected automatically via `CopyrightCrypto::GetApiKey()` so the plugin can start without manual key entry.

### Changes
- **New config folder** - Plugin configuration is now stored in `PalworldDiscordConfig\config.json` (next to the DLL) instead of a flat `config.json` in the working directory. This avoids filename conflicts and makes the config easier to locate.
- **Lua mod always updated** - `main.lua` and `mod.txt` are overwritten on every plugin start to ensure the latest version is always active.
- **Lua reads config from new path** - The Lua bridge now reads event toggles from `PalworldDiscordConfig/config.json` instead of the old mod-internal path.
- **Package script added** - New `package.ps1` builds a ready-to-deploy ZIP (`PalworldDiscordPlugin_v4.0.3.zip`) with correct folder structure. Run `.\package.ps1` after a build.
- **Updated `deploy.ps1`** - Config is deployed to `PalworldDiscordConfig\config.json`.

### Installation (after update)
1. Extract ZIP to `Pal\Binaries\Win64\`
2. Add `PalworldDiscordBridge : 1` to `ue4ss\Mods\mods.txt`
3. Enter your Discord webhook URL in `PalworldDiscordConfig\config.json`
4. Restart server

---

## v4.0.2 (2026-06-23)

### Security & Licensing
- **Copyright protection** - Hardcoded XOR-encrypted API key, copyright, product name and non-commercial notice
- **Remote copyright check** - Plugin validates against `https://rl-dev.de/api/copyright-check` at startup
- **API key validation** - Plugin refuses to start if the API key in `config.json` does not match the hardcoded value

### Changes
- **HTTP server removed** - Plugin no longer starts a local web server; Discord → game uses the file bridge only
- **HTTP client improved** - Reads response body on non-2xx status codes to parse server error reasons
- **README updated** - Removed API key details from public documentation

---

## v4.0.0 (2026-06-18)

### Major Changes
- **File-based IPC bridge** - Complete rewrite of Lua ↔ C++ communication
- **Discord Bot (two-way chat)** - Discord messages forwarded to in-game chat (`discord_to_game`, `bot_token`, `channel_id`)
- **Multi-language support** - English and German (`"language": "en"` or `"de"`)
- **Configurable bot name** - `"bot_name"` in config
- **Event toggles** - Enable/disable chat, join, leave, death events individually
- **System message filter** - PalDefender/system messages are filtered out

### Removed
- ConsoleReader / LogWatcher fallback
- `package.loadlib` DLL exports (`SendChatToDiscord`, `SendEmbedToDiscord`)
- Hardcoded webhook URL from source code

---

## v3.2 (Previous)

- Added event toggles (chat, join, leave, death)
- File bridge IPC prototype
- Debug logging enhancements
- Join hook disabled (caused disconnects)
