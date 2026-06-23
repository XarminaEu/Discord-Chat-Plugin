# UE4SS Integration Guide (Game-Chat-Hooking)

Dieses Dokument beschreibt, wie das Plugin in Palworld eingebunden wird, damit
es den echten Spiel-Chat liest und beschreibt (wie PalDefender, das ebenfalls
auf UE4SS aufsetzt).

> **Wichtig:** Der Plugin-Kern (Discord-Webhook + HTTP-Server + File-Bridge)
> ist fertig und getestet. Dieser Schritt fügt die *Spiel-Anbindung* hinzu.

---

## Übersicht: Zwei Integrations-Optionen

| Option | Methode | Aufwand | Status |
|---|---|---|---|
| **A** | **UE4SS Lua-Bridge** (empfohlen) | Kopieren + Konfigurieren | **Bereit** |
| **B** | UE4SS C++ Mod (komplex) | Kompilieren + SDK | Vorhanden, nicht getestet |

**Empfehlung:** Nutze **Option A**. Sie ist einfacher, benötigt keinen
Build-Prozess und läuft mit der UE4SS-Runtime, die du bereits hast.

---

# Option A: Lua-Bridge (empfohlen)

Die Lua-Bridge kommuniziert über Dateien mit dem C++-Plugin:

- `bridge_out.txt` — Lua schreibt Spiel-Chat, C++ liest → Discord
- `bridge_in.txt` — C++ schreibt Discord-Nachrichten, Lua liest → Spiel

## Schritt A1: Dateien auf den Server kopieren

Kopiere das Lua-Mod aus dem Plugin-Projekt in den UE4SS-Mods-Ordner:

```powershell
# Quelle: Plugin-Projekt
$src = "D:\Palworld\PalworldDiscordPlugin\ue4ss_lua"
# Ziel: UE4SS Runtime (auf dem Server)
$dst = "D:\Palworld\UE4SS_SDK\Mods\PalworldDiscordBridge"

# Ordner anlegen
New-Item -ItemType Directory -Force -Path $dst | Out-Null
New-Item -ItemType Directory -Force -Path "$dst\Scripts" | Out-Null
New-Item -ItemType Directory -Force -Path "$dst\dlls" | Out-Null

# Lua-Mod kopieren
Copy-Item "$src\mod.txt"        "$dst\mod.txt"
Copy-Item "$src\Scripts\main.lua" "$dst\Scripts\main.lua"

# C++ Plugin-DLL kopieren
Copy-Item "D:\Palworld\PalworldDiscordPlugin\build\bin\Release\PalworldDiscordPlugin.dll" `
          "$dst\dlls\PalworldDiscordPlugin.dll"
```

Ergebnis auf dem Server:

```
UE4SS_SDK/Mods/PalworldDiscordBridge/
├── mod.txt
├── Scripts/
│   └── main.lua
└── dlls/
    └── PalworldDiscordPlugin.dll
```

## Schritt A2: Mod aktivieren

Füge in `UE4SS_SDK/Mods/mods.txt` eine Zeile hinzu:

```
PalworldDiscordBridge : 1
```

## Schritt A3: Konfiguration anpassen

Öffne `Scripts/main.lua` und passe die obersten Werte an:

```lua
local CONFIG = {
    -- Pfade (relativ zum Spiel-Root; oder absolut)
    bridge_in_file  = "PalDiscordBridge_in.txt",
    bridge_out_file = "PalDiscordBridge_out.txt",

    -- Pfad zur Plugin-DLL (relativ zum Spiel-Verzeichnis)
    dll_path = "Mods/PalworldDiscordBridge/dlls/PalworldDiscordPlugin.dll",

    -- Wenn leer, scannt das Skript automatisch nach Chat-Funktionen.
    -- Du kannst hier einen Hinweis eintragen, z.B. "Chat" oder "Say".
    chat_function_hint = "",

    -- Property-Namen in der Chat-UFunction (werden automatisch probiert)
    message_property = "Message",
    sender_property  = "SenderName",

    -- Discord -> Spiel
    send_chat_function_hint = "",
    poll_interval = 500,
    discord_prefix = "[Discord] ",
}
```

> **Wichtig:** Stelle sicher, dass die `bridge_*_file`-Pfade identisch mit den
> Werten in `config.json` (`bridge.incoming_file` / `bridge.outgoing_file`)
> sind!

## Schritt A4: Plugin-Config erstellen

Lege eine `config.json` im Spiel-Root (`Pal/Binaries/Win64/`) ab:

```json
{
  "discord": {
    "webhook_url": "https://discord.com/api/webhooks/..."
  },
  "plugin": {
    "debug_mode": true,
    "log_file": "PalworldDiscordPlugin.log",
    "http_port": 8765,
    "http_bind": "127.0.0.1",
    "api_key": ""
  },
  "bridge": {
    "enabled": true,
    "incoming_file": "PalDiscordBridge_in.txt",
    "outgoing_file": "PalDiscordBridge_out.txt"
  }
}
```

## Schritt A5: Chat-Funktionen ermitteln (einmalig)

Da Palworld-Funktionsnamen sich zwischen Builds ändern können, findest du sie
mit dem **UE4SS Live View** heraus:

1. Starte den Server mit UE4SS (Debug-Konsole aktiv).
2. Öffne die UE4SS-GUI (Standard: `{`-Taste) → Tab **Live View**.
3. Sende im Spiel eine Chat-Nachricht und beobachte, welche **UFunction**
   feuert (alternativ den **Function Hooker** nutzen).
4. Trage den gefundenen Namen in `main.lua` unter `chat_function_hint` ein.
   Beispiele: `"ReceiveChatMessage"`, `"ServerSay"`, `"ChatToServer"`.

> **Tipp:** Lass `chat_function_hint` leer beim ersten Start. Das Skript
> scannt automatisch und schreibt gefundene Kandidaten ins UE4SS-Log.

## Schritt A6: Verifizieren

- Server starten → UE4SS-Log zeigt:
  `[PalworldDiscordBridge] DLL geladen...`
  und `[PalworldDiscordBridge] Hook registriert...`.
- Schreibe im Spiel-Chat → Nachricht erscheint in Discord.
- Sende vom Bot eine HTTP-POST an `http://server-ip:8765/discord/message` →
  Nachricht erscheint im Spiel-Chat mit `[Discord]`-Prefix.

---

# Option B: C++ Mod (fortgeschritten)

Falls du tiefe UE4SS-Integration ohne Datei-Bridge brauchst, kannst du den
C++-Mod in `src/ue4ss_mod.cpp` kompilieren. Das erfordert das RE-UE4SS-
Quell-Repo und Visual Studio.

## Voraussetzungen

- **Unreal-Engine-Source-Zugang**: Epic-Account mit GitHub verknüpft.
- Git, CMake, Visual Studio 2022, ausreichend Speicher.

## Schritt B1: RE-UE4SS klonen

```powershell
cd D:\Palworld\PalworldDiscordPlugin\ue4ss
git clone https://github.com/UE4SS-RE/RE-UE4SS
cd RE-UE4SS
git submodule update --init --recursive
```

> Der `git submodule`-Schritt benötigt den Unreal-Source-Zugang.

## Schritt B2: Plugin als UE4SS-Mod bauen

```powershell
cd D:\Palworld\PalworldDiscordPlugin\ue4ss
& "C:\Program Files\CMake\bin\cmake.exe" -B build -G "Visual Studio 17 2022"
& "C:\Program Files\CMake\bin\cmake.exe" --build build --config Game__Shipping__Win64
```

Ergebnis: `ue4ss/build/.../PalworldDiscordPlugin.dll` mit den Exports
`start_mod` / `uninstall_mod`.

> Build-Config **`Game__Shipping__Win64`** ist Pflicht (UE4SS-Konvention).

## Schritt B3: Deployment

```
Pal/Binaries/Win64/ue4ss/Mods/PalworldDiscordPlugin/
├── dlls/
│   └── main.dll          <- umbenannte PalworldDiscordPlugin.dll
├── config.json
└── enabled.txt
```

1. `PalworldDiscordPlugin.dll` in `dlls/main.dll` umbenennen.
2. `config.json` (mit `ue4ss`-Sektion) in den Mod-Ordner kopieren.
3. Leere `enabled.txt` erstellen.

## Schritt B4: Konfiguration

```json
{
  "ue4ss": {
    "chat_recv_function": "ReceiveChatMessage",
    "chat_message_param": "Message",
    "chat_sender_param": "SenderName",
    "chat_send_function": "BroadcastChatMessage"
  }
}
```

> Trage die echten Namen ein (ermittelt via Live View).

---

## Bekannte Einschränkungen / Hinweise

- **Discord → Spiel** (Injection) ist am stärksten game-spezifisch. Hat die
  echte UFunction eine andere Signatur als erwartet, muss der Code
  (Lua `BroadcastToGame` oder C++ `BroadcastToGame`) angepasst werden.
- Bei Palworld-Updates können sich Funktions-/Property-Namen ändern → dann
  die `chat_function_hint` in `main.lua` (Option A) bzw. `config.json`
  (Option B) aktualisieren.
- Der HTTP-Server bindet standardmäßig auf `127.0.0.1:8765`. Prüfe mit
  `netsh interface ipv4 show excludedportrange protocol=tcp`, ob der Port frei
  ist; sonst in `config.json` ändern.
- **Webhook-URLs sind Geheimnisse**. Teile sie nicht und speichere sie nicht
  unverschlüsselt in öffentlichen Repositories.
