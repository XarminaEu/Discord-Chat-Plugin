-- PalworldDiscordPlugin - UE4SS Lua Bridge
-- Bidirektionale Datei-Bridge zwischen UE4SS Lua und dem C++ Plugin.
-- Spiel-Chat -> bridge_out.txt (Plugin liest, sendet an Discord)
-- Discord -> bridge_in.txt  (Plugin schreibt, Lua liest, injected in Spiel)

-- ========================================================================
-- KONFIGURATION (kann vor dem ersten Start angepasst werden)
-- ========================================================================
local CONFIG = {
    -- Pfade relativ zum Spiel-Root (oder absolut)
    bridge_in_file  = "PalDiscordBridge_in.txt",   -- C++ schreibt, Lua liest
    bridge_out_file = "PalDiscordBridge_out.txt",  -- Lua schreibt, C++ liest

    -- Pfad zur Plugin-DLL. Relativ zum Spiel-Verzeichnis.
    -- Wenn du deploy.ps1 nutzt, liegt die DLL direkt in Win64.
    dll_path = "PalworldDiscordPlugin.dll",

    -- Chat-UFunction Name (Substring-Match). Leer lassen fuer Auto-Scan.
    -- Bekannte Kandidaten: "Say", "ServerSay", "ChatToServer", "SendChat",
    --                      "ReceiveChatMessage", "OnChatMessage"
    chat_function_hint = "",

    -- Property-Namen im Parameter-Struct der Chat-Funktion
    message_property = "Message",      -- oder "ChatMessage", "MessageStr", "Text"
    sender_property  = "SenderName",   -- oder "PlayerName", "Sender", "Name"

    -- UFunction zum Senden von Chat-Nachrichten (fuer Discord -> Spiel)
    -- Leer lassen fuer Auto-Scan
    send_chat_function_hint = "",

    -- Polling-Intervall fuer eingehende Datei (ms)
    poll_interval = 500,

    -- Prefix fuer Discord-Nachrichten im Spiel-Chat
    discord_prefix = "[Discord] ",
}

-- ========================================================================
-- HILFSFUNKTIONEN
-- ========================================================================
local function Log(msg)
    print("[PalworldDiscordBridge] " .. msg)
end

local function Err(msg)
    print("[PalworldDiscordBridge] ERROR: " .. msg)
end

-- Konvertiere Lua-String zu breitem String fuer UE4 (FString)
-- UE4SS Lua uebergibt Lua-Strings automatisch als FString
local function ToFString(str)
    return str
end

-- ========================================================================
-- PLUGIN-DLL LADEN
-- ========================================================================
local function LoadPlugin()
    local full_dll_path = CONFIG.dll_path
    -- Versuche absoluten Pfad aus Spiel-Verzeichnis
    local game_dirs = IterateGameDirectories()
    if game_dirs and game_dirs.Game then
        local game_root = game_dirs.Game.__absolute_path
        -- Entferne /Binaries/Win64 falls vorhanden, um auf Root zu kommen
        full_dll_path = game_root .. "/" .. CONFIG.dll_path
    end

    Log("Versuche DLL zu laden: " .. full_dll_path)
    local ok, loader = pcall(package.loadlib, full_dll_path, "StartBridge")
    if ok and loader then
        local result = loader()
        Log("DLL geladen, StartBridge() = " .. tostring(result))
        return true
    else
        Err("DLL konnte nicht geladen werden: " .. tostring(loader))
        return false
    end
end

-- ========================================================================
-- DATEI-HILFEN (bridge I/O)
-- ========================================================================
-- Lua-Datei-I/O ist blockierend; wir halten die Files nur kurz offen.
local last_in_offset = 0

local function WriteBridgeOut(author, message)
    local f = io.open(CONFIG.bridge_out_file, "a")
    if not f then
        Err("Kann bridge_out nicht oeffnen: " .. CONFIG.bridge_out_file)
        return
    end
    -- Sanitize: Tabs/Newlines in Leerzeichen umwandeln
    local a = author:gsub("[\t\n\r]", " ")
    local m = message:gsub("[\t\n\r]", " ")
    f:write(a .. "\t" .. m .. "\n")
    f:close()
end

local function ReadBridgeIn()
    local f = io.open(CONFIG.bridge_in_file, "r")
    if not f then
        return nil  -- noch nicht erstellt
    end
    local size = f:seek("end")
    if size < last_in_offset then
        -- Datei wurde zurueckgesetzt
        last_in_offset = 0
    end
    if size == last_in_offset then
        f:close()
        return nil
    end
    f:seek("set", last_in_offset)
    local data = f:read("*a")
    last_in_offset = size
    f:close()

    local lines = {}
    for line in data:gmatch("([^\r\n]+)") do
        if #line > 0 then
            table.insert(lines, line)
        end
    end
    return lines
end

-- ========================================================================
-- CHAT-HOOK (Spiel -> Discord)
-- ========================================================================
local registered_hooks = {}

local function OnChatMessage(context, params)
    -- params ist ein RemoteUnrealParam-Struct; wir greifen via Reflection
    local msg = ""
    local sender = "Player"

    -- Versuche Message-Property zu lesen
    if params and params.Message then
        msg = params.Message:ToString() or ""
    elseif params and params.ChatMessage then
        msg = params.ChatMessage:ToString() or ""
    elseif params and params.Text then
        msg = params.Text:ToString() or ""
    end

    -- Versuche Sender-Property zu lesen
    if params and params.SenderName then
        sender = params.SenderName:ToString() or "Player"
    elseif params and params.PlayerName then
        sender = params.PlayerName:ToString() or "Player"
    elseif params and params.Sender then
        sender = params.Sender:ToString() or "Player"
    elseif context then
        sender = context:GetName() or "Player"
    end

    if msg and #msg > 0 then
        WriteBridgeOut(sender, msg)
    end
end

-- Scannt nach UFunctions, die den Hint im Namen enthalten.
local function FindChatFunction(hint)
    if not hint or hint == "" then
        return nil
    end

    local candidates = {}
    local function Scan()
        ForEachUObject(function(obj)
            if obj:type() == "UFunction" then
                local name = obj:GetName()
                if name:find(hint, 1, true) then
                    table.insert(candidates, name)
                end
            end
        end)
    end

    local ok = pcall(Scan)
    if not ok then
        Err("UObject-Scan fehlgeschlagen")
        return nil
    end

    if #candidates > 0 then
        Log("Gefundene Chat-Funktionen: " .. table.concat(candidates, ", "))
        return candidates[1]
    end
    return nil
end

local function RegisterChatHook()
    local func_name = CONFIG.chat_function_hint

    -- Auto-Scan wenn kein Hint gesetzt
    if not func_name or func_name == "" then
        local hints = { "Chat", "Say", "Message", "SendChat", "ReceiveChat" }
        for _, h in ipairs(hints) do
            func_name = FindChatFunction(h)
            if func_name then
                Log("Auto-Scan waehlte: " .. func_name)
                break
            end
        end
    else
        -- Pruefe ob Funktion existiert
        local exists = FindChatFunction(func_name)
        if not exists then
            Err("Konfigurierte Funktion nicht gefunden: " .. func_name)
        end
    end

    if not func_name or func_name == "" then
        Err("Keine Chat-Funktion gefunden. Setze 'chat_function_hint' in main.lua.")
        return false
    end

    Log("Registriere Hook auf: " .. func_name)
    local pre_id, post_id = RegisterHook(func_name, function(self, params)
        -- params ist hier ein RemoteUnrealParam; hol den echten Wert
        local actual_params = params:get()
        OnChatMessage(self, actual_params)
    end)

    table.insert(registered_hooks, { name = func_name, pre = pre_id, post = post_id })
    Log("Hook registriert (pre=" .. tostring(pre_id) .. ", post=" .. tostring(post_id) .. ")")
    return true
end

-- ========================================================================
-- CHAT-INJECTION (Discord -> Spiel)
-- ========================================================================
local send_function_ref = nil
local send_target_ref   = nil

local function FindSendChatFunction()
    local hint = CONFIG.send_chat_function_hint
    if not hint or hint == "" then
        hint = "Say"
    end

    local candidates = {}
    local ok = pcall(function()
        ForEachUObject(function(obj)
            if obj:type() == "UFunction" then
                local name = obj:GetName()
                if name:find(hint, 1, true) then
                    table.insert(candidates, { name = name, obj = obj })
                end
            end
        end)
    end)

    if not ok then return nil end
    if #candidates == 0 then return nil end

    Log("Send-Chat Kandidaten: " .. candidates[1].name)
    return candidates[1].obj
end

local function FindSendTarget(func)
    if not func then return nil end
    local outer = func:GetOuter()
    if not outer then return nil end

    local class_name = outer:GetName()
    local target = FindFirstOf(class_name)
    return target
end

local function BroadcastToGame(author, message)
    if not send_function_ref then
        send_function_ref = FindSendChatFunction()
        if send_function_ref then
            send_target_ref = FindSendTarget(send_function_ref)
        end
    end

    if not send_function_ref or not send_target_ref then
        Err("Keine Send-Chat Funktion/Ziel gefunden. Nachricht verworfen: " .. author .. ": " .. message)
        return false
    end

    local full_msg = CONFIG.discord_prefix .. author .. ": " .. message

    -- Injection muss im Game-Thread passieren
    ExecuteInGameThread(function()
        local params = {}
        params.Message = ToFString(full_msg)
        -- Alternative Property-Namen falls Message nicht existiert
        params.ChatMessage = ToFString(full_msg)
        params.Text = ToFString(full_msg)

        local ok, err = pcall(function()
            send_target_ref:CallFunction(send_function_ref, params)
        end)
        if not ok then
            Err("Injection fehlgeschlagen: " .. tostring(err))
        end
    end)

    return true
end

-- ========================================================================
-- BRIDGE-LOOP (Discord -> Spiel)
-- ========================================================================
local function StartBridgeLoop()
    Log("Starte Bridge-Loop (Intervall: " .. CONFIG.poll_interval .. " ms)")

    LoopAsync(CONFIG.poll_interval, function()
        local lines = ReadBridgeIn()
        if lines then
            for _, line in ipairs(lines) do
                local tab_pos = line:find("\t")
                local author, message
                if tab_pos then
                    author = line:sub(1, tab_pos - 1)
                    message = line:sub(tab_pos + 1)
                else
                    author = "Discord"
                    message = line
                end
                BroadcastToGame(author, message)
            end
        end
        return false  -- false = Loop weiterlaufen lassen
    end)
end

-- ========================================================================
-- INITIALISIERUNG
-- ========================================================================
Log("========================================")
Log("PalworldDiscordPlugin Lua Bridge v1.0")
Log("========================================")

-- 1. Plugin-DLL laden
local dll_loaded = LoadPlugin()
if not dll_loaded then
    Log("Fahre trotzdem mit Lua-Only-Modus fort...")
end

-- 2. Chat-Hook registrieren (Spiel -> Discord)
local hook_ok = RegisterChatHook()
if not hook_ok then
    Log("WARNUNG: Kein Chat-Hook aktiv. Nur Discord -> Spiel funktioniert.")
end

-- 3. Bridge-Loop starten (Discord -> Spiel)
StartBridgeLoop()

Log("Initialisierung abgeschlossen.")
