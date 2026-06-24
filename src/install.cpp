#include "install.h"
#include "logger.h"
#include "copyright_crypto.h"
#include <windows.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace {

// Eingebettete Lua-Hauptdatei
const char* LUA_MAIN = R"PDPDPD(-- PalworldDiscordPlugin v4.0.3 - UE4SS Lua Bridge
-- Auto-installiert vom C++ Plugin. Nicht manuell bearbeiten.

local CONFIG = {
    dll_path = "ue4ss/Mods/PalworldDiscordBridge/dlls/PalworldDiscordPlugin.dll",
}

local function Log(msg)
    print("[Lua] [PalworldDiscordBridge] " .. msg)
end
local function Err(msg)
    print("[Lua] [PalworldDiscordBridge] ERROR: " .. msg)
end

-- ========================================================================
-- DLL Exports
-- ========================================================================
local EVENTS = { chat = true, join = true, leave = true, death = true }
local BRIDGE_FILE = "ue4ss/Mods/PalworldDiscordBridge/dlls/PalDiscordBridge_out.txt"
local BRIDGE_INCOMING_FILE = "ue4ss/Mods/PalworldDiscordBridge/dlls/PalDiscordBridge_in.txt"

local function LoadConfig()
    local ok, json = pcall(require, "json")
    if not ok then
        Log("json module not available, using defaults")
        return
    end
    local path = "PalworldDiscordConfig/config.json"
    local f = io.open(path, "r")
    if not f then
        Log("config.json not found, using defaults")
        return
    end
    local content = f:read("*a")
    f:close()
    local ok2, data = pcall(json.decode, content)
    if ok2 and data and data.events then
        if data.events.chat ~= nil then EVENTS.chat = data.events.chat end
        if data.events.join ~= nil then EVENTS.join = data.events.join end
        if data.events.leave ~= nil then EVENTS.leave = data.events.leave end
        if data.events.death ~= nil then EVENTS.death = data.events.death end
        Log("Config loaded: chat=" .. tostring(EVENTS.chat) .. " join=" .. tostring(EVENTS.join) .. " leave=" .. tostring(EVENTS.leave) .. " death=" .. tostring(EVENTS.death))
    else
        Log("Config parse failed, using defaults")
    end
end

local function WriteBridge(eventType, player, msg)
    local line = eventType .. "|" .. (player or "") .. "|" .. (msg or "") .. "\n"
    local f = io.open(BRIDGE_FILE, "a")
    if f then
        f:write(line)
        f:close()
    else
        Err("Cannot write bridge file")
    end
end

-- Read incoming Discord messages (C++ writes, Lua reads)
local function ReadIncomingBridge()
    local f = io.open(BRIDGE_INCOMING_FILE, "r")
    if not f then return end
    local content = f:read("*a")
    f:close()
    os.remove(BRIDGE_INCOMING_FILE)

    if not content or #content == 0 then return end

    for line in content:gmatch("[^\r\n]+") do
        local p1 = line:find("|")
        local p2 = line:find("|", p1 + 1)
        if p1 and p2 then
            local event = line:sub(1, p1 - 1)
            local sender = line:sub(p1 + 1, p2 - 1)
            local msg = line:sub(p2 + 1)
            if event == "discord" and sender and msg and #msg > 0 then
                Log("Discord->Game: [" .. sender .. "]: " .. msg)
                BroadcastSystemMessage("[Discord] " .. sender .. ": " .. msg)
            end
        end
    end
end

-- ========================================================================
-- Chat Hook: /Script/Pal.PalGameStateInGame:BroadcastChatMessage
-- ========================================================================
local chatHooked = false
local function RegisterChatHook()
    if chatHooked then return true end

    local ok, preId, postId = pcall(RegisterHook,
        "/Script/Pal.PalGameStateInGame:BroadcastChatMessage",
        function(self, ChatMessage)
            local ok2, sender, message = pcall(function()
                local chat = ChatMessage:get()
                local s = chat.Sender:ToString()
                local m = chat.Message:ToString()
                return s, m
            end)
            if ok2 and sender and message and #message > 0 then
                if EVENTS.chat then
                    -- Filter system messages (empty sender = PalDefender/system)
                    if sender == "" or sender:find("SYSTEM") then
                        Log("System message filtered: " .. message:sub(1, 50))
                    else
                        Log("Chat: [" .. sender .. "]: " .. message)
                        WriteBridge("chat", sender, message)
                        Log("Chat bridged to file")
                    end
                else
                    Log("Chat event disabled, skipping")
                end
            end
        end)

    if ok and preId then
        chatHooked = true
        Log("ChatHook OK (BroadcastChatMessage)")
        return true
    else
        Err("ChatHook failed: " .. tostring(preId))
        return false
    end
end

-- ========================================================================
-- Player Join Hook
-- ========================================================================
local joinHooked = false
local join_recent = {} -- deduplication buffer
local function RegisterJoinHook()
    if joinHooked then return true end

    -- Post-Hook: fires AFTER the function completes (no disconnects)
    local ok, preId, postId = pcall(RegisterHook,
        "/Script/Engine.PlayerController:ServerAcknowledgePossession",
        nil, -- no pre-hook (prevents disconnects)
        function(Context, Pawn)
            ExecuteWithDelay(3000, function()
                local ok2, name = pcall(function()
                    local pc = Context:get()
                    if not pc or not pc:IsValid() then return nil end
                    local ps = pc.PlayerState
                    if ps and ps:IsValid() then
                        return ps:GetPlayerName():ToString()
                    end
                    return nil
                end)
                if ok2 and name and #name > 0 then
                    -- deduplicate: same player within 30s
                    local now = os.time()
                    if not join_recent[name] or (now - join_recent[name]) > 30 then
                        join_recent[name] = now
                        if EVENTS.join then
                            Log("Join: " .. name)
                            WriteBridge("join", name, "")
                        else
                            Log("Join event disabled, skipping")
                        end
                    else
                        Log("Join dedup: " .. name)
                    end
                end
            end)
        end)

    if ok and postId then
        joinHooked = true
        Log("JoinHook OK (ServerAcknowledgePossession POST-HOOK + 3s delay)")
        return true
    else
        Log("JoinHook not available (" .. tostring(postId) .. ")")
        return false
    end
end

-- ========================================================================
-- Player Death Hook
-- ========================================================================
local deathHooked = false
local function RegisterDeathHook()
    if deathHooked then return true end

    local hooks = {
        "/Script/Pal.PalGameStateInGame:OnPlayerDied",
        "/Script/Pal.PalCharacter:OnDeath",
    }

    for _, hookPath in ipairs(hooks) do
        local ok, preId, postId = pcall(RegisterHook, hookPath,
            function(self, Param)
                local ok2, name = pcall(function()
                    -- Try PlayerState directly
                    local ps = Param:get()
                    if ps and ps:IsValid() and ps.GetPlayerName then
                        return ps:GetPlayerName():ToString()
                    end
                    -- Fallback: try getting from self (the game state)
                    local gs = self:get()
                    if gs and gs:IsValid() then
                        -- OnPlayerDied might have different params
                        -- Try to extract from the param as UObject
                        local obj = Param:get()
                        if obj and obj:IsValid() then
                            if obj.GetPlayerName then
                                return obj:GetPlayerName():ToString()
                            end
                        end
                    end
                    return nil
                end)
                if ok2 and name and #name > 0 then
                    if EVENTS.death then
                        Log("Death: " .. name)
                        WriteBridge("death", name, "")
                    else
                        Log("Death event disabled, skipping")
                    end
                else
                    Log("DeathHook: could not extract name from " .. hookPath)
                end
            end)
        if ok then
            deathHooked = true
            Log("DeathHook OK (" .. hookPath .. ")")
            return true
        end
    end

    Log("DeathHook not available")
    return false
end

-- ========================================================================
-- Player Leave Hook
-- ========================================================================
local leaveHooked = false
local function RegisterLeaveHook()
    if leaveHooked then return true end

    -- Try various disconnect hooks
    local hooks = {
        "/Script/Pal.PalGameStateInGame:OnRemovePlayer",
        "/Script/Pal.PalGameStateInGame:OnLogout",
    }

    for _, hookPath in ipairs(hooks) do
        local ok, preId, postId = pcall(RegisterHook, hookPath,
            function(self, Param)
                local ok2, name = pcall(function()
                    -- Try PlayerState directly
                    local ps = Param:get()
                    if ps and ps:IsValid() and ps.GetPlayerName then
                        return ps:GetPlayerName():ToString()
                    end
                    -- Fallback: OnRemovePlayer sometimes passes Controller
                    local ctrl = Param:get()
                    if ctrl and ctrl:IsValid() then
                        if ctrl.PlayerState and ctrl.PlayerState:IsValid() then
                            return ctrl.PlayerState:GetPlayerName():ToString()
                        end
                        if ctrl.GetPlayerName then
                            return ctrl:GetPlayerName():ToString()
                        end
                    end
                    return nil
                end)
                if ok2 and name and #name > 0 then
                    if EVENTS.leave then
                        Log("Leave: " .. name)
                        WriteBridge("leave", name, "")
                    else
                        Log("Leave event disabled, skipping")
                    end
                else
                    Log("LeaveHook: could not extract name from " .. hookPath)
                end
            end)
        if ok then
            leaveHooked = true
            Log("LeaveHook OK (" .. hookPath .. ")")
            return true
        end
    end

    Log("LeaveHook not available")
    return false
end

-- ========================================================================
-- Ingame System Message via Broadcast (rcon)
-- ========================================================================
local function BroadcastSystemMessage(text)
    -- Use UE4SS ExecuteConsoleCommand if available
    if ExecuteConsoleCommand then
        local ok = pcall(ExecuteConsoleCommand, "Broadcast " .. text)
        if ok then
            Log("System message sent (ExecuteConsoleCommand): " .. text)
            return true
        end
    end

    -- Fallback: get player controller and use ConsoleCommand
    local ok, pc = pcall(FindFirstOf, "PalPlayerController")
    if ok and pc and pc:IsValid() then
        local ok2 = pcall(function()
            pc:ConsoleCommand("Broadcast " .. text, true)
        end)
        if ok2 then
            Log("System message sent (ConsoleCommand): " .. text)
            return true
        end
    end

    -- Fallback 2: try any PlayerController
    local ok3, allPC = pcall(FindAllOf, "PlayerController")
    if ok3 and allPC then
        for _, p in pairs(allPC) do
            if p and p:IsValid() then
                local ok4 = pcall(function()
                    p:ConsoleCommand("Broadcast " .. text, true)
                end)
                if ok4 then
                    Log("System message sent (FindAllOf): " .. text)
                    return true
                end
            end
        end
    end

    Log("System message could not be sent - no player controller available")
    return false
end

-- ========================================================================
-- Main
-- ========================================================================
Log("v4.0.3 Lua Bridge")
Log("Hooks: Chat (BroadcastChatMessage), Join (POST-HOOK), Leave, Death")

-- Load event toggles from config.json
LoadConfig()

-- Initialize C++ plugin (loads config, webhook, etc)
Log("Initializing C++ plugin...")
local startBridge = nil
local ok0, fn0 = pcall(package.loadlib, CONFIG.dll_path, "StartBridge")
if ok0 and fn0 then startBridge = fn0 end
if startBridge then
    local ok_init = pcall(startBridge)
    if ok_init then
        Log("C++ plugin initialized")
    else
        Err("C++ plugin init failed")
    end
else
    Err("StartBridge export not found")
end

Log("Using file bridge: " .. BRIDGE_FILE)

-- Register all UE4SS hooks (game -> Discord via file bridge)
Log("Registering hooks...")
RegisterChatHook()
RegisterJoinHook()
RegisterLeaveHook()
RegisterDeathHook()

-- Start periodic reader for Discord->Game messages (C++ writes file, Lua reads)
Log("Starting Discord->Game bridge reader...")
if LoopAsync then
    LoopAsync(2000, function()
        ReadIncomingBridge()
    end)
else
    Log("LoopAsync not available - Discord->Game bridge will not work")
end

-- Send system message after short delay (allow game to fully load)
ExecuteWithDelay(10000, function()
    BroadcastSystemMessage("Chat System online [Alpha]")
end)

Log("Ready.")
)PDPDPD";

const char* MOD_TXT =
"PalworldDiscordBridge\n"
"PalworldDiscordBridge v4.0.3\n"
"Discord <-> Palworld chat bridge\n"
"PalworldDiscordPlugin\n"
"4.0.3\n";

const char* DEFAULT_CONFIG =
"{\n"
"  \"discord\": {\n"
"    \"webhook_url\": \"\",\n"
"    \"bot_token\": \"\",\n"
"    \"channel_id\": \"\",\n"
"    \"bot_name\": \"Server Chat\",\n"
"    \"language\": \"en\",\n"
"    \"discord_to_game\": false,\n"
"    \"poll_interval\": 5\n"
"  },\n"
"  \"plugin\": {\n"
"    \"debug_mode\": true,\n"
"    \"log_file\": \"PalworldDiscordPlugin.log\",\n"
"    \"http_port\": 8765,\n"
"    \"http_bind\": \"127.0.0.1\",\n"
"    \"max_message_length\": 2000,\n"
"    \"api_key\": \"\"\n"
"  },\n"
"  \"events\": {\n"
"    \"chat\": true,\n"
"    \"join\": true,\n"
"    \"leave\": true,\n"
"    \"death\": true\n"
"  },\n"
"  \"bridge\": {\n"
"    \"enabled\": true,\n"
"    \"incoming_file\": \"PalDiscordBridge_in.txt\",\n"
"    \"outgoing_file\": \"PalDiscordBridge_out.txt\"\n"
"  }\n"
"}\n";

bool WriteTextFile(const std::string& path, const char* content) {
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    f << content;
    return f.good();
}

bool DirExists(const std::string& path) {
    DWORD attr = GetFileAttributesA(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool FileExists(const std::string& path) {
    DWORD attr = GetFileAttributesA(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool CreateDirRecursive(const std::string& path) {
    if (path.empty()) return false;
    size_t pos = 0;
    while ((pos = path.find_first_of("/\\", pos + 1)) != std::string::npos) {
        std::string sub = path.substr(0, pos);
        CreateDirectoryA(sub.c_str(), nullptr);
    }
    CreateDirectoryA(path.c_str(), nullptr);
    return DirExists(path);
}

} // namespace

bool SelfInstall(const std::string& game_root) {
    bool installed_any = false;

    // 1. PalworldDiscordConfig-Ordner + config.json erstellen (nur wenn nicht vorhanden)
    std::string config_dir = game_root + "\\PalworldDiscordConfig";
    CreateDirRecursive(config_dir);
    std::string config_path = config_dir + "\\config.json";
    if (!FileExists(config_path)) {
        std::string default_config = DEFAULT_CONFIG;
        std::string placeholder = "\"api_key\": \"\"";
        std::string actual = "\"api_key\": \"" + CopyrightCrypto::GetApiKey() + "\"";
        size_t pos = default_config.find(placeholder);
        if (pos != std::string::npos) {
            default_config.replace(pos, placeholder.size(), actual);
        }
        if (WriteTextFile(config_path, default_config.c_str())) {
            std::cout << "[PalworldDiscordPlugin] config.json erstellt: " << config_path << std::endl;
            installed_any = true;
        } else {
            std::cerr << "[PalworldDiscordPlugin] WARNUNG: Konnte config.json nicht erstellen." << std::endl;
        }
    }

    // 2. UE4SS Lua-Mod Dateien erstellen, wenn nicht vorhanden
    std::string mod_dir = game_root + "\\ue4ss\\Mods\\PalworldDiscordBridge";
    std::string scripts_dir = mod_dir + "\\Scripts";
    std::string lua_path = scripts_dir + "\\main.lua";
    std::string modtxt_path = mod_dir + "\\mod.txt";

    // Always overwrite Lua files to ensure latest version
    if (!CreateDirRecursive(mod_dir)) {
        std::cerr << "[PalworldDiscordPlugin] WARNUNG: Konnte Mod-Verzeichnis nicht erstellen: " << mod_dir << " (Error: " << GetLastError() << ")" << std::endl;
    }
    if (!CreateDirRecursive(scripts_dir)) {
        std::cerr << "[PalworldDiscordPlugin] WARNUNG: Konnte Scripts-Verzeichnis nicht erstellen: " << scripts_dir << " (Error: " << GetLastError() << ")" << std::endl;
    }

    // Force-delete old lua to ensure update works even if file is locked
    if (GetFileAttributesA(lua_path.c_str()) != INVALID_FILE_ATTRIBUTES) {
        if (!DeleteFileA(lua_path.c_str())) {
            std::cerr << "[PalworldDiscordPlugin] WARNUNG: Konnte alte main.lua nicht loeschen (Error: " << GetLastError() << ")" << std::endl;
        }
    }

    if (WriteTextFile(lua_path, LUA_MAIN)) {
        std::cout << "[PalworldDiscordPlugin] Lua-Mod aktualisiert: " << lua_path << std::endl;
        installed_any = true;
    } else {
        std::cerr << "[PalworldDiscordPlugin] WARNUNG: Konnte main.lua nicht schreiben! (Error: " << GetLastError() << ")" << std::endl;
    }

    if (WriteTextFile(modtxt_path, MOD_TXT)) {
        std::cout << "[PalworldDiscordPlugin] mod.txt aktualisiert." << std::endl;
        installed_any = true;
    } else {
        std::cerr << "[PalworldDiscordPlugin] WARNUNG: Konnte mod.txt nicht schreiben! (Error: " << GetLastError() << ")" << std::endl;
    }

    // 3. mods.txt aktualisieren (Hinweis fuer User)
    std::string mods_txt = game_root + "\\ue4ss\\Mods\\mods.txt";
    if (GetFileAttributesA(mods_txt.c_str()) != INVALID_FILE_ATTRIBUTES) {
        std::ifstream in(mods_txt);
        std::stringstream buffer;
        buffer << in.rdbuf();
        std::string content = buffer.str();
        if (content.find("PalworldDiscordBridge") == std::string::npos) {
            std::ofstream out(mods_txt, std::ios::app);
            out << "\nPalworldDiscordBridge : 1\n";
            std::cout << "[PalworldDiscordPlugin] mods.txt aktualisiert." << std::endl;
            installed_any = true;
        }
    }

    return installed_any;
}
