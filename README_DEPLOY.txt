PalworldDiscordPlugin - Deploy-Pack
=====================================

SCHRITTE:
1. Kopiere PalworldDiscordPlugin.dll nach:
   Pal/Binaries/Win64/

2. Starte den Server.

3. Die DLL erstellt automatisch:
   - config.json (im Win64-Ordner)
   - ue4ss/Mods/PalworldDiscordBridge/Scripts/main.lua
   - Aktiviert sich in mods.txt

4. Oeffne die erstellte config.json und trage ein:
   - webhook_url: Deine Discord-Webhook-URL

5. Server neustarten. Fertig.

DISCORD WEBHOOK TEST:
- Setze die URL in config.json
- Schreibe im Spiel-Chat -> Discord erhaelt die Nachricht
- POST an http://server-ip:8765/discord/message -> Spiel erhaelt Nachricht
