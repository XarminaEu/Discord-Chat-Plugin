# Changelog

## v4.1.6 (2026-07-02)

### Bug Fixes
- **Dedicated-Server Crash-Resistenz** — automatischer Safe Mode, wenn `LoopAsync` oder `ExecuteConsoleCommand` nicht verfügbar sind (WindowsGSM / einige UE4SS-Setups).
  - `ApplyServerOptimizations` wird übersprungen.
  - `OptimizeBaseActors` wird übersprungen.
  - `BroadcastSystemMessage` berührt auf Dedicated Servern keine `PlayerController`-Objekte mehr.
  - Startup-Systemnachricht wird übersprungen.
  - `JoinHook` führt den Join-Handler direkt aus, wenn `ExecuteWithDelay` im Safe Mode fehlt.

### Changes
- Neue globale Lua-Variable `DEDICATED_COMPAT` zur Laufzeit-Erkennung.
- Version bump auf **v4.1.6**.

---

## v4.1.5 (2026-07-02)

### New Features
- **Web Console Savegames Tab** — Zeigt Spieler-Savegame-Dateien aus `Saved/SaveGames/0/*/Players/*.sav` an.
  - Scant rekursiv nach `Players`-Ordnern in den ueblichen Dedicated-Server Pfaden.
  - Zeigt die SteamID/UID (Dateiname) und den vollen Pfad an.
  - **Delete-Button** pro Savegame, der die `.sav` Datei loescht (Sicherheits-Check: nur Dateien unter einem `Players`-Ordner).
  - Auto-Refresh alle 30 Sekunden.

### API
- Neue Endpunkte: `/api/savegames`, `/api/deletesave`

### Changes
- Version bump auf **v4.1.5**.

---

## v4.1.4 (2026-07-02)

### New Features
- **Web Console Player/Banlist Tabs** — die eingebettete Web RCON Console hat jetzt Tabs:
  - **Console**: RCON-Befehle wie bisher
  - **Online**: Live-Spielerliste via `listplayers` mit Kick-/Ban-Buttons
  - **Offline**: Spieler aus dem Cache (`player_cache.json`) mit Steam-/Platform-ID und Ban-Button
  - **Banlist**: Gebannte Spieler via `banlist` mit Unban-Button
  - Alle Spieler-Listen werden alle 30 Sekunden synchronisiert (Auto-Refresh)

### API
- Neue Endpunkte: `/api/players`, `/api/offline`, `/api/banlist`, `/api/action`

### Changes
- Version bump auf **v4.1.4**.

---

## v4.1.3 (2026-07-02)

### New Features
- **Scheduled Auto-Restart (`auto_restart`)** — automatischer Server-Neustart alle N Stunden, ausgerichtet an der Systemzeit (z.B. 00:00, 06:00, 12:00, 18:00 für `interval_hours: 6`).
  - Sendet Discord-Warnung X Minuten vor dem Restart (`warning_minutes`).
  - Sendet finale Discord-Nachricht beim Start des Restarts.
  - Schreibt ein Signal-File, damit die Lua-Mod versucht `SaveGame`/`save` auszuführen.
  - Beendet danach den Server-Prozess via `ExitProcess(0)`. Ein externer Wrapper (WindowsGSM, Batch, systemd, etc.) startet den Server neu.
  - Optional: `restart_script` Pfad zu einer Batch/EXE, die kurz vor dem Beenden gestartet wird (z.B. um Wrapper zu benachrichtigen).

### Config
- Neue `auto_restart` Sektion:
  - `enabled` (default `false`), `interval_hours` (default `6`), `warning_minutes` (default `5`), `send_webhook` (default `true`), `restart_script` (default `""`).

### Changes
- Neue Klasse `RestartManager` (`include/restart_manager.h`, `src/restart_manager.cpp`).
- Version bump auf **v4.1.3**.

---

## v4.1.2 (2026-07-02)

### New Features
- **Engine/Network Tweaks (`engine_tweaks`)** — neue Config-Sektion für fortgeschrittene Server-Optimierung, wenn FPS bei vielen Basen/Spielern einbricht.
  - **Engine.ini Patches** (C++ Plugin): `NetServerMaxTickRate`, `MaxNetTickRate`, `MaxInternetClientRate`, `MaxNetUpdateRate`, `NetClientTicksPerSecond`, `GameNetworkManager` Bandwidth-Limits, `ConfiguredInternetSpeed`, `ConfiguredLanSpeed`, `GarbageCollectionSettings.TimeBetweenPurgingPendingKillObjects`.
  - **UE4SS Lua Base-Actor Throttling**: Alle 30 Sekunden werden Build-/Base-Camp-Actors auf niedrigere `NetUpdateFrequency` (default 5 Hz) gedrosselt. Player-Actors bleiben auf 60 Hz. Deaktivierbar via `optimize_base_actors`.

### Config
- Neue `engine_tweaks` Sektion in `config.json` (default: `enabled: false`).
  - `enabled`, `net_server_max_tick_rate`, `max_net_tick_rate`, `max_internet_client_rate`, `total_net_bandwidth`, `max_dynamic_bandwidth`, `min_dynamic_bandwidth`, `configured_internet_speed`, `configured_lan_speed`, `max_net_update_rate`, `net_client_ticks_per_second`, `gc_time_between_purging`, `optimize_base_actors`, `base_net_update_frequency`, `player_net_update_frequency`, `optimize_tick_interval`.

### Changes
- `json.h`: Unterstützung für `double`/`float` Werte (`as_double`, `get_double`) hinzugefügt.
- Version bump auf **v4.1.2**.

---

## v4.1.1 (2026-07-02)

### Bug Fixes
- **Doppelter Join/Leave/Death-Spam behoben** — `ConsoleReader` und `LogWatcher` haben beide dieselben Server-Log-Zeilen gelesen und Events doppelt an Discord geschickt. Der `ConsoleReader` wurde entfernt; nur noch `LogWatcher` wertet die Server-Logs aus.
- **Dedup-Zeit zu lang** — Das interne Deduplizierungsfenster lag bei 300 Sekunden und unterdrückte damit echte Events (z. B. ein Spieler joined/leaved mehrfach innerhalb von 5 Minuten). Reduziert auf **15 Sekunden**, was Duplikate aus mehreren Quellen abfängt, aber echte Reconnects korrekt meldet.

### Removed
- `ConsoleReader` Klasse und Member aus `plugin.cpp` / `plugin.h` entfernt.

### Changes
- Version bump auf **v4.1.1**.

---

## v4.1.0 (2026-07-01)

### New Features
- **Embedded Web RCON Console** — ein kleiner HTTP-Server in der DLL, der eine browserbasierte RCON-Konsole auf Port 8080 (konfigurierbar) bereitstellt. Er verbindet sich mit dem PalDefender RCON-Server (Standard 127.0.0.1:25575) und dient als Web-Proxy.
  - Login mit dem in der Config hinterlegten Admin-Passwort (`web_console.password`).
  - Terminal-ähnliches UI mit Befehlshistorie (Pfeiltasten).
  - Session-Token-Authentifizierung, Logout-Button.
  - Endpunkte: `/api/login`, `/api/cmd`, `/api/logout`.

### Config
- Neue `web_console` Sektion in `config.json`:
  - `enabled` (default `false`)
  - `port` (default `8080`)
  - `password` (UI-Login-Passwort, leer = deaktiviert aus Sicherheit)
  - `rcon_host` (default `127.0.0.1`)
  - `rcon_port` (default `25575`)
  - `rcon_password` (PalDefender RCON-Passwort)

### Security
- Web Console wird **nicht** gestartet, wenn `password` leer ist, auch wenn `enabled: true` gesetzt ist.

### Changes
- Version bump auf **v4.1.0** in C++, Lua, Mod-Metadaten, Package-Script und README/CHANGELOG.
- `config.json` Template: `debug_mode` auf `false` gesetzt und `events`/`web_console` Sektionen hinzugefügt.

---

## v4.0.9 (2026-07-01)

### Bug Fixes
- **`ResolvePlayer` UID-Regex fehlerhaft** — `^[0-9A-Fa-f%-]+` hatte kein Endezeichen `$`, daher wurden Strings wie `"abc123xyz"` fälschlicherweise als UID erkannt. Jetzt: `^[0-9A-Fa-f%-]+$`.
- **`TryGetPlayerUid` falsche Fallback-Methode** — `player_state:GetName()` gibt den internen UE-Objektnamen zurück (`PlayerState_0`), nicht die Spieler-UID. Entfernt.
- **Leave-Hook toter Code** — `ctrl.GetPlayerName` war immer `nil` wenn der vorherige `obj.GetPlayerName`-Check bereits fehlgeschlagen war. Logik bereinigt und korrekt auf PlayerState vs. Controller-Objekttyp aufgeteilt.
- **`PerformRemoteCopyrightCheck` doppelte Variable** — `program` und `product` enthielten beide `GetProductName()`. Eine Variable entfernt.
- **`SavePlayerCache` unnötige Kopie** — `player_cache` wurde ohne Grund in ein neues `data`-Table kopiert. Direkt enkodiert.

### Removed
- **`BroadcastToGameChat` C++ Export** — war ein leerer Stub, der nichts tat und nie von Lua aufgerufen wurde.
- **`debug_mode: true` als Default** — war für frische Installs schlecht (sehr viele Log-Zeilen). Jetzt `false`.
- **`http_port` / `http_bind` in Default-Config** — HTTP-Server wurde in v4.0.2 entfernt; die toten Optionen blieben im Template. Jetzt entfernt.
- **`[Alpha]` in Discord-Startup-Embed-Titel** — veraltet, entfernt.
- **Identische `de ? "..." : "..."` Ternaries für Embed-Footer** — alle Branches gaben `"Palworld Discord Bridge"` zurück. Durch direktes Literal ersetzt.
- **Doppeltes Log in `SendEmbed`** — Debug- und Info-Log mit identischem Inhalt. Debug-Zeile entfernt.
- **Ungenutztes `size_t pos = 0` in `LooksLikeIpAddress`** — deklariert aber nie verwendet.

### Changes
- **Default-Config erweitert** — `server_performance` enthält jetzt auch `network_tick_rate` und `boost_priority`.

---

## v4.0.8 (2026-07-01)

### New Features
- **RCON admin commands** (via PalDefender / UE4SS console):
  - `paldiscord.reload` — reloads `PalworldDiscordConfig/config.json` without server restart.
  - `paldiscord.delaccount <name|steamid|uid>` — deletes the player save file (`Players/<uid>.sav`).
  - `paldiscord.delbase <name|steamid|uid>` — destroys the player's build objects in-game (only works while the player is online).
- **Player ID logging** — on every join the plugin logs `name`, `PlayerUID`, `SteamID` and platform ID to:
  - `PalworldDiscordConfig/player_log.txt`
  - `PalworldDiscordConfig/player_cache.json`
- **Server performance options extended**:
  - `network_tick_rate` config option (default `120`).
  - `boost_priority` config option (default `false`) — raises server process priority to HIGH (use with caution).
  - Engine.ini patch now also writes `MaxNetTickRate` and `MaxInternetClientRate` under `[IpNetDriver]`.
  - Additional Lua runtime commands: `net.MaxRepArraySize`, `net.MaxRepArrayMemory`, `ai.SpawnCycle`, `wp.Runtime.MaxConsecutiveOverdueFrames`.

### Changes
- **Version bump** — all C++, Lua, mod metadata and package strings now report `v4.0.8`.
- **README updated** — added `mods.json` / `enabled.txt` activation note for newer UE4SS builds.

### Installation / Upgrade Notes
- Extract the new ZIP to `Pal/Binaries/Win64/`.
- Add the mod to the UE4SS mod list:
  - Legacy UE4SS: `Win64/ue4ss/Mods/mods.txt` → add `PalworldDiscordBridge : 1`
  - Newer UE4SS (Mods.json): `Win64/ue4ss/Mods/mods.json` → set `PalworldDiscordBridge` to `true` / enabled
- Restart the server.

---

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
