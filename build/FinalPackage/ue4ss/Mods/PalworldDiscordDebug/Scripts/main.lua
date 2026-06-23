-- PalworldDiscordPlugin - DEBUG/DIAGNOSE MOD
-- Aktiviere in mods.txt: PalworldDiscordDebug : 1
-- Suche im Server-Log nach "[DEBUG]".

local function Log(msg)
    print("[DEBUG] " .. msg)
end

Log("========================================")
Log("  PALWORLDDISCORD DEBUG MOD GESTARTET")
Log("========================================")

-- 1. Spiel-Verzeichnis
Log("1. SPIEL-VERZEICHNIS:")
local game_dirs = nil
local game_root = nil
if IterateGameDirectories then
    game_dirs = IterateGameDirectories()
    if game_dirs and game_dirs.Game then
        game_root = game_dirs.Game.__absolute_path
        Log("   Game Root = " .. tostring(game_root))
    else
        Log("   Game Root = NICHT GEFUNDEN")
    end
else
    Log("   IterateGameDirectories NICHT VERFUEGBAR")
end

-- 2. Lua-Umgebung
Log("2. LUA-UMGEBUNG:")
Log("   _VERSION = " .. tostring(_VERSION))

-- 3. UE4SS-Funktionen
Log("3. UE4SS FUNKTIONEN:")
local ue4ss_funcs = { "RegisterHook", "ExecuteInGameThread", "FindFirstOf", "ForEachUObject", "IterateGameDirectories", "LoopAsync" }
for _, name in ipairs(ue4ss_funcs) do
    Log("   " .. name .. " = " .. (_G[name] ~= nil and "VERFUEGBAR" or "FEHLT"))
end

-- 4. Dateien
Log("4. DATEI-PRUEFUNG:")
local files = {
    "config.json",
    "PalworldDiscordPlugin.dll",
    "ue4ss/Mods/PalworldDiscordBridge/mod.txt",
    "ue4ss/Mods/PalworldDiscordBridge/Scripts/main.lua",
    "ue4ss/Mods/PalworldDiscordBridge/dlls/PalworldDiscordPlugin.dll",
}
for _, path in ipairs(files) do
    local f = io.open(path, "r")
    if f then f:close(); Log("   [OK]  " .. path) else Log("   [X]   " .. path) end
end

-- 5. mods.txt
Log("5. MODS.TXT:")
local f = io.open("ue4ss/Mods/mods.txt", "r")
if f then
    local content = f:read("*a")
    f:close()
    if content:find("PalworldDiscord") then
        Log("   [OK]  Enthaelt 'PalworldDiscord'")
    else
        Log("   [X]   Kein 'PalworldDiscord' Eintrag!")
    end
else
    Log("   [X]   mods.txt nicht gefunden")
end

-- 6. DLL-Test
Log("6. DLL-LADETEST:")
local ok, loader = pcall(package.loadlib, "ue4ss/Mods/PalworldDiscordBridge/dlls/PalworldDiscordPlugin.dll", "StartBridge")
if ok and type(loader) == "function" then
    Log("   [OK]  DLL Export 'StartBridge' gefunden")
else
    Log("   [X]   DLL-Ladung fehlgeschlagen")
end

Log("========================================")
Log("  DEBUG ENDE")
Log("========================================")
