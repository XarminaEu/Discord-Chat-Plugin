PalworldDiscordPlugin v2.0 - LogWatcher Edition
================================================

NUR DATEIEN KOPIEREN - KEINE SKRIPTE NOTWENDIG

SCHRITT 1: Dateien kopieren
----------------------------
Kopiere diese Dateien in deinen Pal/Binaries/Win64 Ordner:

1. PalworldDiscordPlugin.dll         -> Win64/
2. config.json                       -> Win64/
3. Der ganze Ordner ue4ss/           -> Win64/ue4ss/

SCHRITT 2: Mod aktivieren
--------------------------
Oeffne die Datei Win64/ue4ss/Mods/mods.txt mit einem Texteditor.
Fuege am Ende dieser Datei eine neue Zeile ein:

    PalworldDiscordBridge : 1

SCHRITT 3: Discord-Webhook eintragen
-------------------------------------
Oeffne die Datei Win64/config.json mit einem Texteditor.
Trage deine Discord-Webhook-URL ein:

    "webhook_url": "https://discord.com/api/webhooks/DEINE_URL_HIER"

SCHRITT 4: Server starten
--------------------------
Starte den Palworld-Server neu. Fertig.

WIE ES FUNKTIONIERT
-------------------
Die DLL ueberwacht den Server-Log (Pal/Saved/Logs/).
Wenn ein Spieler im Chat schreibt, erkennt die DLL das
und sendet es sofort an deinen Discord-Webhook.

WICHTIG: Kein HTTP-Server, kein Lua-Bridge
-------------------------------------------
- Der HTTP-Server ist deaktiviert (kein Crash unter Wine/Proton)
- Die Lua-Datei laedt nur die DLL, der Rest passiert in C++
- Discord -> Spiel geht aktuell nicht (nur Spiel -> Discord)

FEHLERBEHEBUNG
--------------
1. Pruefe mods.txt auf: PalworldDiscordBridge : 1
2. Pruefe config.json auf korrekte Webhook-URL
3. Suche im Server-Log nach "[PalworldDiscordPlugin]"
4. Pruefe ob Pal/Saved/Logs/ existiert (Log-Dateien noetig)

DATEIEN
-------
PalworldDiscordPlugin.dll            -> Haupt-DLL
config.json                          -> Konfiguration (Webhook-URL)
ue4ss/Mods/PalworldDiscordBridge/    -> Lua-Loader (nur DLL laden)
