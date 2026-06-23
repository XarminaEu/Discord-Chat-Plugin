# PalworldDiscordPlugin - Installation Fix
# Fuehre dies im Pal/Binaries/Win64 Ordner aus.
# Es prueft alle Dateien und legt fehlende an.

param(
    [string]$Win64Path = $PSScriptRoot
)

$ErrorActionPreference = "Stop"

if (-not $Win64Path) {
    Write-Host "[FEHLER] Fuehre das Skript aus dem Win64-Ordner heraus aus." -ForegroundColor Red
    exit 1
}

$Win64Path = $Win64Path.TrimEnd('\','/')
Write-Host "[*] Pruefe Installation in: $Win64Path" -ForegroundColor Cyan

$dllPath = Join-Path $Win64Path "PalworldDiscordPlugin.dll"
$ue4ssPath = Join-Path $Win64Path "ue4ss"
$modsPath = Join-Path $ue4ssPath "Mods"
$modDir = Join-Path $modsPath "PalworldDiscordBridge"
$scriptsDir = Join-Path $modDir "Scripts"
$luaPath = Join-Path $scriptsDir "main.lua"
$modTxt = Join-Path $modDir "mod.txt"
$modsTxt = Join-Path $modsPath "mods.txt"
$found_issue = $false

# 1. DLL pruefen
if (-not (Test-Path $dllPath)) {
    Write-Host "[FEHLER] DLL fehlt: $dllPath" -ForegroundColor Red
    Write-Host "         Kopiere PalworldDiscordPlugin.dll hierher!" -ForegroundColor Yellow
    $found_issue = $true
} else {
    $size = (Get-Item $dllPath).Length
    Write-Host "[OK]   DLL vorhanden ($([math]::Round($size/1KB,1)) KB)" -ForegroundColor Green
}

# 2. Lua-Mod pruefen
if (-not (Test-Path $luaPath)) {
    Write-Host "[FEHLER] Lua-Mod fehlt: $luaPath" -ForegroundColor Red
    Write-Host "         Erstelle Lua-Mod automatisch..." -ForegroundColor Yellow
    
    New-Item -ItemType Directory -Force -Path $scriptsDir | Out-Null
    
    $luaContent = @'-- PalworldDiscordPlugin - UE4SS Lua Bridge
local CONFIG = {
    bridge_in_file  = "PalDiscordBridge_in.txt",
    bridge_out_file = "PalDiscordBridge_out.txt",
    dll_path = "PalworldDiscordPlugin.dll",
    chat_function_hint = "",
    message_property = "Message",
    sender_property  = "SenderName",
    send_chat_function_hint = "",
    poll_interval = 500,
    discord_prefix = "[Discord] ",
}

local function Log(msg)
    print("[PalworldDiscordBridge] " .. msg)
end
local function Err(msg)
    print("[PalworldDiscordBridge] ERROR: " .. msg)
end
local function ToFString(str)
    return str
end

local function LoadPlugin()
    local full_dll_path = CONFIG.dll_path
    local game_dirs = IterateGameDirectories()
    if game_dirs and game_dirs.Game then
        local game_root = game_dirs.Game.__absolute_path
        full_dll_path = game_root .. "/" .. CONFIG.dll_path
    end
    Log("Lade DLL: " .. full_dll_path)
    local ok, loader = pcall(package.loadlib, full_dll_path, "StartBridge")
    if ok and loader then
        local result = loader()
        Log("DLL geladen = " .. tostring(result))
        return true
    else
        Err("DLL-Fehler: " .. tostring(loader))
        return false
    end
end

local last_in_offset = 0
local function WriteBridgeOut(author, message)
    local f = io.open(CONFIG.bridge_out_file, "a")
    if not f then return end
    local a = author:gsub("[\t\n\r]", " ")
    local m = message:gsub("[\t\n\r]", " ")
    f:write(a .. "\t" .. m .. "\n")
    f:close()
end

local function ReadBridgeIn()
    local f = io.open(CONFIG.bridge_in_file, "r")
    if not f then return nil end
    local size = f:seek("end")
    if size < last_in_offset then last_in_offset = 0 end
    if size == last_in_offset then f:close() return nil end
    f:seek("set", last_in_offset)
    local data = f:read("*a")
    last_in_offset = size
    f:close()
    local lines = {}
    for line in data:gmatch("([^\r\n]+)") do
        if #line > 0 then table.insert(lines, line) end
    end
    return lines
end

local function OnChatMessage(context, params)
    local msg = ""
    local sender = "Player"
    if params and params.Message then
        msg = params.Message:ToString() or ""
    elseif params and params.ChatMessage then
        msg = params.ChatMessage:ToString() or ""
    elseif params and params.Text then
        msg = params.Text:ToString() or ""
    end
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

local function FindChatFunction(hint)
    if not hint or hint == "" then return nil end
    local candidates = {}
    local ok = pcall(function()
        ForEachUObject(function(obj)
            if obj:type() == "UFunction" then
                local name = obj:GetName()
                if name:find(hint, 1, true) then
                    table.insert(candidates, name)
                end
            end
        end)
    end)
    if not ok then return nil end
    if #candidates > 0 then
        Log("Gefunden: " .. table.concat(candidates, ", "))
        return candidates[1]
    end
    return nil
end

local registered_hooks = {}
local function RegisterChatHook()
    local func_name = CONFIG.chat_function_hint
    if not func_name or func_name == "" then
        local hints = { "Chat", "Say", "Message", "SendChat", "ReceiveChat" }
        for _, h in ipairs(hints) do
            func_name = FindChatFunction(h)
            if func_name then
                Log("Auto-Scan: " .. func_name)
                break
            end
        end
    end
    if not func_name or func_name == "" then
        Err("Keine Chat-Funktion gefunden.")
        return false
    end
    Log("Hook: " .. func_name)
    local pre_id, post_id = RegisterHook(func_name, function(self, params)
        local actual_params = params:get()
        OnChatMessage(self, actual_params)
    end)
    table.insert(registered_hooks, { name = func_name, pre = pre_id, post = post_id })
    Log("Hook OK (pre=" .. tostring(pre_id) .. ")")
    return true
end

local send_function_ref = nil
local send_target_ref   = nil
local function FindSendChatFunction()
    local hint = CONFIG.send_chat_function_hint
    if not hint or hint == "" then hint = "Say" end
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
    if not ok or #candidates == 0 then return nil end
    Log("Send-Kandidat: " .. candidates[1].name)
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
        Err("Kein Send-Ziel. Verworfen: " .. author .. ": " .. message)
        return false
    end
    local full_msg = CONFIG.discord_prefix .. author .. ": " .. message
    ExecuteInGameThread(function()
        local params = {}
        params.Message = ToFString(full_msg)
        params.ChatMessage = ToFString(full_msg)
        params.Text = ToFString(full_msg)
        local ok, err = pcall(function()
            send_target_ref:CallFunction(send_function_ref, params)
        end)
        if not ok then Err("Injection: " .. tostring(err)) end
    end)
    return true
end

local function StartBridgeLoop()
    Log("Bridge-Loop gestartet")
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
        return false
    end)
end

Log("PalworldDiscordPlugin Lua Bridge v1.0")
local dll_loaded = LoadPlugin()
if not dll_loaded then
    Log("Lua-Only Modus...")
end
local hook_ok = RegisterChatHook()
if not hook_ok then
    Log("WARNUNG: Kein Hook. Nur Discord->Spiel.")
end
StartBridgeLoop()
Log("Bereit.")
'@
    
    Set-Content -Path $luaPath -Value $luaContent -Encoding UTF8
    Write-Host "[OK]   Lua-Mod erstellt." -ForegroundColor Green
    $found_issue = $true
} else {
    Write-Host "[OK]   Lua-Mod vorhanden." -ForegroundColor Green
}

# 3. mod.txt pruefen
if (-not (Test-Path $modTxt)) {
    Write-Host "[INFO] mod.txt fehlt, erstelle..." -ForegroundColor Yellow
    @'
PalworldDiscordBridge
PalworldDiscordBridge v1.0
Discord <-> Palworld chat bridge
PalworldDiscordPlugin
1.0
'@ | Set-Content -Path $modTxt -Encoding UTF8
    $found_issue = $true
} else {
    Write-Host "[OK]   mod.txt vorhanden." -ForegroundColor Green
}

# 4. mods.txt pruefen
$hasEntry = $false
if (Test-Path $modsTxt) {
    $content = Get-Content $modsTxt -Raw
    if ($content -match "PalworldDiscordBridge") {
        $hasEntry = $true
    }
}
if (-not $hasEntry) {
    Write-Host "[INFO] Fuege PalworldDiscordBridge zu mods.txt hinzu..." -ForegroundColor Yellow
    if (-not (Test-Path $modsTxt)) {
        "PalworldDiscordBridge : 1" | Set-Content -Path $modsTxt -Encoding UTF8
    } else {
        Add-Content -Path $modsTxt -Value "PalworldDiscordBridge : 1" -Encoding UTF8
    }
    Write-Host "[OK]   mods.txt aktualisiert." -ForegroundColor Green
    $found_issue = $true
} else {
    Write-Host "[OK]   mods.txt enthaelt Eintrag." -ForegroundColor Green
}

# 5. config.json pruefen
$configPath = Join-Path $Win64Path "config.json"
if (-not (Test-Path $configPath)) {
    Write-Host "[INFO] config.json fehlt, erstelle mit Defaults..." -ForegroundColor Yellow
    @'{
  "discord": {
    "webhook_url": ""
  },
  "plugin": {
    "debug_mode": true,
    "log_file": "PalworldDiscordPlugin.log",
    "http_port": 8765,
    "http_bind": "127.0.0.1",
    "max_message_length": 2000,
    "api_key": ""
  },
  "bridge": {
    "enabled": true,
    "incoming_file": "PalDiscordBridge_in.txt",
    "outgoing_file": "PalDiscordBridge_out.txt"
  }
}
'@ | Set-Content -Path $configPath -Encoding UTF8
    Write-Host "[OK]   config.json erstellt (bitte Webhook-URL eintragen!)." -ForegroundColor Yellow
    $found_issue = $true
} else {
    Write-Host "[OK]   config.json vorhanden." -ForegroundColor Green
}

# Ergebnis
Write-Host ""
if ($found_issue) {
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "DATEIEN WURDEN ANGELEGT / AKTUALISIERT" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "Starte den Server jetzt neu!" -ForegroundColor White
} else {
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "ALLE DATEIEN SIND OK" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "Wenn die DLL trotzdem nicht laedt:" -ForegroundColor White
    Write-Host "  1. Pruefe das UE4SS-Log auf Fehler." -ForegroundColor Gray
    Write-Host "  2. Stelle sicher, dass ue4ss laeuft." -ForegroundColor Gray
    Write-Host "  3. Pruefe: Windows Event Viewer oder ue4ss/Logs/" -ForegroundColor Gray
}

Write-Host ""
Write-Host "WICHTIG: Du musst noch deine Discord-Webhook-URL in config.json eintragen!" -ForegroundColor Yellow
Write-Host "Pfad: $configPath" -ForegroundColor Gray
