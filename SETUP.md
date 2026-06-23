# Setup-Anleitung für Palworld Discord Plugin

## Status der Installation

### ✅ Bereits erledigt:
- Visual Studio Build Tools 2022 (C++ Compiler) - **INSTALLIERT**
- Projekt-Struktur erstellt
- CMakeLists.txt konfiguriert
- Quellcode-Dateien erstellt
- VSCode-Konfiguration vorbereitet

### ⏳ In Arbeit:
- CMake 3.29.3 wird installiert...

### ⏹️ Noch zu tun:
- vcpkg installieren (für Dependencies)
- Dependencies mit vcpkg installieren
- Projekt mit CMake bauen

---

## Schritt 1: CMake Installation abwarten

CMake wird gerade installiert. Nach Abschluss:
```powershell
cmake --version
```

Sollte die Version anzeigen.

---

## Schritt 2: vcpkg installieren

```powershell
# Navigiere zu einem Verzeichnis deiner Wahl (z.B. C:\dev)
cd C:\dev

# Clone vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat

# Integriere vcpkg in Visual Studio
.\vcpkg integrate install
```

---

## Schritt 3: Dependencies installieren

```powershell
# Navigiere zum vcpkg-Verzeichnis
cd C:\dev\vcpkg

# Installiere die benötigten Libraries
.\vcpkg install curl:x64-windows
.\vcpkg install nlohmann-json:x64-windows
.\vcpkg install sqlite3:x64-windows
```

Dies kann 10-15 Minuten dauern.

---

## Schritt 4: Projekt konfigurieren

```powershell
cd D:\Palworld\PalworldDiscordPlugin

# Erstelle build-Verzeichnis
mkdir build
cd build

# Konfiguriere mit CMake (ersetze PATH_TO_VCPKG)
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\dev\vcpkg\scripts\buildsystems\vcpkg.cmake -G "Visual Studio 17 2022"
```

---

## Schritt 5: Projekt bauen

```powershell
cd D:\Palworld\PalworldDiscordPlugin\build

# Baue im Release-Modus
cmake --build . --config Release
```

Die DLL wird dann in `build\bin\PalworldDiscordPlugin.dll` erstellt.

---

## Schritt 6: In VSCode bauen (Alternative)

1. Öffne den Ordner `D:\Palworld\PalworldDiscordPlugin` in VSCode
2. Drücke `Ctrl+Shift+P`
3. Gib ein: `CMake: Configure`
4. Wähle `Visual Studio 17 2022` als Generator
5. Drücke `Ctrl+Shift+P` erneut
6. Gib ein: `CMake: Build`

---

## Troubleshooting

### CMake nicht gefunden
```powershell
# Starte PowerShell neu oder:
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
cmake --version
```

### cl.exe nicht gefunden
```powershell
# Starte die Visual Studio Developer Command Prompt:
"C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
```

### vcpkg Integration fehlgeschlagen
```powershell
cd C:\dev\vcpkg
.\vcpkg integrate install
```

### Dependencies nicht gefunden
```powershell
# Stelle sicher, dass du den korrekten Pfad zu vcpkg nutzt:
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\dev\vcpkg\scripts\buildsystems\vcpkg.cmake
```

---

## Nächste Schritte

Nach erfolgreichem Build:

1. Kopiere `build\bin\PalworldDiscordPlugin.dll` zu:
   ```
   C:\Program Files\Palworld\Binaries\Win64\
   ```

2. Kopiere `config\config.json` zu:
   ```
   C:\Program Files\Palworld\Binaries\Win64\
   ```

3. Bearbeite `config.json` mit deinen Discord-Daten

4. Starte Palworld Server neu

---

## Weitere Dokumentation

- `plan.md` - Implementierungsplan
- `BOT_INTEGRATION.md` - Discord Bot Integration
- `README.md` - Projekt-Übersicht
