# Palworld Discord Chat Plugin - Implementierungsplan

## Projektübersicht
Reines C++ DLL-Server-Plugin für Palworld, das Chat-Nachrichten bidirektional mit einem Discord-Server synchronisiert.

**Architektur:**
- Native C++ DLL-Plugin (keine Mod-Datei)
- UE4SS C++ Modding API Integration
- Discord Webhook Integration (ausgehend)
- Discord Bot Integration (eingehend)
- Lokale SQLite-Datenbank für Logging/Caching

---

## Phase 1: Projekt-Setup
- [ ] **1.1** Ordnerstruktur erstellen
  - `src/` - Quellcode
  - `include/` - Header-Dateien
  - `lib/` - Externe Libraries (curl, json, sqlite3)
  - `build/` - Build-Output
  - `config/` - Konfigurationsdateien
  
- [ ] **1.2** CMakeLists.txt erstellen
  - UE4SS SDK als Abhängigkeit
  - libcurl für HTTP-Requests
  - nlohmann/json für JSON-Parsing
  - sqlite3 für Datenbank
  
- [ ] **1.3** Visual Studio Projekt-Konfiguration
  - DLL-Export-Definitionen
  - Richtige Include-Pfade
  - Linker-Einstellungen für UE4SS

---

## Phase 2: Core Plugin-Struktur
- [ ] **2.1** Plugin-Einstiegspunkt (DllMain)
  - Initialisierung bei DLL-Load
  - UE4SS Hook-System registrieren
  
- [ ] **2.2** Palworld Chat-Hook implementieren
  - Chat-Event-Listener für Spieler-Nachrichten
  - Spieler-Name und Nachrichtstext extrahieren
  - Broadcast-Funktion hooken
  
- [ ] **2.3** Konfigurationssystem
  - `config.json` laden
  - Discord Webhook URL
  - Discord Bot Token
  - Server-ID und Channel-ID
  - Plugin-Einstellungen (Debug-Modus, etc.)

---

## Phase 3: Discord Webhook Integration (Chat → Discord)
- [ ] **3.1** HTTP-Client mit libcurl
  - POST-Requests an Discord Webhooks
  - Error-Handling und Retry-Logik
  - Rate-Limiting beachten
  
- [ ] **3.2** Nachrichtenformatierung
  - Embeds für bessere Formatierung
  - Spieler-Name, Nachricht, Timestamp
  - Optional: Spieler-Level/Rang anzeigen
  
- [ ] **3.3** Asynchrone Verarbeitung
  - Thread-Pool für HTTP-Requests
  - Keine Blockierung des Game-Threads

---

## Phase 4: Discord Bot Integration (Discord → Chat)
**HINWEIS:** Discord Bot wird extern erweitert (nicht Teil dieses Plugins)

- [ ] **4.1** Bot-zu-Plugin Kommunikation (Plugin-Seite)
  - Eingehende Datei-Bridge (`bridge_in.txt`)
  - Zeilenformat: `discord|Autor|Nachricht`
  - Lua-Mod liest Datei und broadcastet im Spiel-Chat

- [ ] **4.2** Chat-Broadcast im Spiel
  - Nachrichten in Palworld-Chat einfügen
  - Format: `[Discord] Benutzername: Nachricht`
  - Farbcodierung (optional)
  
- [ ] **4.3** Bot-Erweiterung (EXTERN - dein bestehendes Projekt)
  - Message-Listener im konfigurierten Channel
  - Schreibe Zeile in `bridge_in.txt`
  - Format: `discord|username|message`

---

## Phase 5: Datenbank & Logging
- [ ] **5.1** SQLite-Datenbank Setup
  - Tabelle für Chat-Logs
  - Tabelle für Fehler/Debug-Logs
  
- [ ] **5.2** Logging-System
  - Alle Chat-Nachrichten loggen
  - Fehler und Warnungen tracken
  - Log-Rotation implementieren

---

## Phase 6: Testing & Deployment
- [ ] **6.1** Unit-Tests
  - JSON-Parsing testen
  - HTTP-Request-Handling testen
  - Config-Loading testen
  
- [ ] **6.2** Integration-Tests
  - Plugin-Laden testen
  - Chat-Hook testen
  - Discord-Integration testen
  
- [ ] **6.3** Deployment
  - DLL in Palworld-Verzeichnis kopieren
  - Config-Datei bereitstellen
  - Discord Bot Token sicher speichern
  - Dokumentation erstellen

---

## Technische Details

### DLL-Platzierung
```
Palworld/Binaries/Win64/Plugins/PalworldDiscordPlugin.dll
```

### Config-Datei (config.json)
```json
{
  "discord": {
    "webhook_url": "https://discord.com/api/webhooks/...",
    "bot_token": "YOUR_BOT_TOKEN",
    "server_id": "123456789",
    "channel_id": "987654321"
  },
  "plugin": {
    "debug_mode": true,
    "log_file": "PalworldDiscordPlugin.log",
    "max_message_length": 2000
  }
}
```

### UE4SS Integration Points
- `UObject` Hook für Chat-System
- `FString` für Text-Verarbeitung
- `FTimerManager` für asynchrone Tasks
- `FOutputDevice` für Logging

---

## Abhängigkeiten
- **UE4SS SDK** - C++ Modding API
- **libcurl** - HTTP-Requests
- **nlohmann/json** - JSON-Parsing
- **sqlite3** - Datenbank
- **Discord.py/Discord.js** - Bot (separates Projekt)

---

## Sicherheitsüberlegungen
- [ ] Discord Bot Token nicht hardcoden
- [ ] Config-Datei mit eingeschränkten Berechtigungen
- [ ] Input-Validierung für Chat-Nachrichten
- [ ] Rate-Limiting für Discord API
- [ ] Fehlerbehandlung ohne Crash des Servers

---

## Zeitschätzung
- Phase 1-2: 2-3 Tage (Setup & Core)
- Phase 3: 2-3 Tage (Webhook)
- Phase 4: 3-4 Tage (Bot Integration)
- Phase 5: 1-2 Tage (Datenbank)
- Phase 6: 2-3 Tage (Testing & Deployment)
- **Gesamt: 10-15 Tage**

---

## Nächste Schritte
1. Ordnerstruktur und CMakeLists.txt erstellen
2. UE4SS SDK herunterladen und konfigurieren
3. Plugin-Einstiegspunkt (DllMain) implementieren
4. Chat-Hook entwickeln
