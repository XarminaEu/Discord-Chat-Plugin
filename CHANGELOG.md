# Changelog

## v4.0.7 (2026-06-28)

### New Features
- **Server Performance Optimization** - Plugin automatically fixes the common "server FPS drops to 4" issue on startup:
  - **Engine.ini auto-patch** (persistent): Writes `NetServerMaxTickRate=120` and `bUseFixedFrameRate=False` to `Pal/Saved/Config/WindowsServer/Engine.ini` on first start. Takes effect after the next server restart.
  - **Runtime console commands** (immediate): Executes `t.MaxFPS 120`, `p.MaxSubstepDeltaTime 0.01`, and `net.MaxRPCPerNetUpdate 64` via UE4SS 5 seconds after startup.
  - Configurable via `config.json` under `server_performance` (`enabled`, `target_fps`, `patch_ini`).

### Bug Fixes
- **Lua crash fixed**: `p1 + 1` arithmetic on nil in bridge file parser — guard added in both source `main.lua` and embedded Lua.
- **Lua crash fixed**: `ExecuteWithDelay` called without nil-guard — now safely skipped if unavailable.
- **Duplicate join/leave/death events** suppressed: 15-second deduplication window prevents double Discord notifications when both the Lua hook and the log/console watcher fire for the same event.
- **Wrong version string**: `Initialize()` logged "v3.2" — corrected to v4.0.7.
- **LogWatcher error spam**: No longer logs ERROR every 5 seconds when the Logs directory doesn't exist yet.
- **C++ unsigned overflow**: `line.find('|', npos+1)` — `npos` check now happens before the second find.
- **NULL HMODULE under Wine/Proton**: `StartBridge` now falls back to `GetModuleHandleExA` with `GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS` if `GetModuleHandleA` returns NULL; DllMain HMODULE is stored and reused.

---

## v4.0.6 (2026-06-28)

### Bug Fixes
- **Join/Leave/Death events now work** - The Lua hooks for join/leave/death could not find the right Unreal functions in the current Palworld build. The plugin now uses the server console/log output to detect these events reliably.
- **ConsoleReader and LogWatcher re-enabled** - Both watchers are started again and feed join/leave/death events directly to the Discord webhook. Chat is still handled by the Lua hook to avoid duplicates.
- **Log path auto-detection** - LogWatcher defaults to `Pal/Saved/Logs` relative to the DLL directory.
- **Player name extraction improved** - Extractor tries UE4SS quoted names, single-quoted names, `UserId=...` and `steam_...` identifiers.
- **Version consistency** - All hardcoded version strings (C++, Lua, mod metadata) now report `v4.0.6`.

---

## v4.0.5 (2026-06-28)

### Bug Fixes
- **CurseForge upload compatibility** - The `dlls/` folder in the release ZIP is no longer empty, preventing CurseForge's "General error processing file".

---

## v4.0.4 (2026-06-28)

### Bug Fixes
- **Fixed bridge file path mismatch** - Lua and C++ now both use `ue4ss/Mods/PalworldDiscordBridge/dlls/PalDiscordBridge_out.txt` by default.
- **Empty `dlls/` folder included** - Release ZIP contains the `dlls/` subfolder so the Lua bridge can write the file immediately.

---

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
