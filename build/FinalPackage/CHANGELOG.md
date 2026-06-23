# Changelog

## v4.0.0 (2026-06-18)

### Major Changes
- **File-based IPC bridge** - Complete rewrite of Lua <-> C++ communication
  - Removed unreliable `package.loadlib` DLL export approach
  - Lua writes events to file, C++ reads and sends to Discord
  - Much more reliable and debuggable
- **Removed ConsoleReader and LogWatcher** - Only file bridge is used now
  - Eliminates duplicate/conflicting event sources
  - Cleaner architecture

### Features
- **Discord Bot (two-way chat)** - Discord messages forwarded to in-game chat
  - Set `discord_to_game: true` + `bot_token` + `channel_id` in config
  - C++ polls Discord API, Lua broadcasts messages in-game
  - No RCON required
- **Multi-language support** - English and German
  - Set `"language": "en"` or `"de"` in `config.json`
  - All Discord embeds are translated (startup, join, leave, death)
- **Configurable bot name** - `"bot_name": "Server Chat"` in config
- **Event toggles** - Enable/disable chat, join, leave, death events individually
- **System message filter** - PalDefender/system messages are filtered out
- **Custom webhook URL** - No hardcoded URL, user must configure their own

### Changes
- Webhook URL removed from default config (must be set by user)
- `config.json.example` provided as reference
- `CONFIG.md` with full configuration documentation
- English README with installation instructions
- Startup notification and system messages cleaned up

### Removed
- ConsoleReader fallback (console buffer reading)
- LogWatcher fallback (log file monitoring)
- `package.loadlib` DLL exports (`SendChatToDiscord`, `SendEmbedToDiscord`)
- Hardcoded webhook URL from source code

---

## v3.2 (Previous)

- Added event toggles (chat, join, leave, death)
- Added config-based settings
- File bridge IPC prototype
- Debug logging enhancements
- Join hook disabled (caused disconnects)

