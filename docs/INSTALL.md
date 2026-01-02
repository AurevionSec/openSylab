# OpenSylab v0.2 - Installation und Kompilierung

## Systemanforderungen

### Betriebssysteme
- Linux (Ubuntu 20.04+ empfohlen)
- Windows 10/11 (mit MinGW oder Visual Studio)
- macOS (experimentell)

### Erforderliche Software

1. **C++ Compiler**
   - GCC 9+ oder Clang 10+ (Linux/macOS)
   - Visual Studio 2019+ oder MinGW-w64 (Windows)

2. **CMake**
   - Version 3.15 oder höher

3. **SQLite3**
   - Entwicklungsbibliotheken erforderlich

4. **Git** (zum Klonen des Repositories)

## Installation der Abhängigkeiten

### Ubuntu/Debian

```bash
sudo apt update
sudo apt install -y build-essential cmake git libsqlite3-dev
```

### Fedora/RHEL

```bash
sudo dnf install -y gcc-c++ cmake git sqlite-devel
```

### macOS (mit Homebrew)

```bash
brew install cmake sqlite3
```

### Windows

#### Option 1: Visual Studio

1. Visual Studio 2019 oder neuer installieren
2. "Desktop development with C++" Workload auswählen
3. CMake über Visual Studio Installer hinzufügen

#### Option 2: MinGW-w64

1. MSYS2 installieren von https://www.msys2.org/
2. MSYS2 Terminal öffnen und ausführen:
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-sqlite3 make
```

## Projekt klonen

```bash
git clone https://github.com/yourusername/openSylab.git
cd openSylab
```

## Kompilierung

### Linux / macOS

```bash
# Build-Verzeichnis erstellen
mkdir build
cd build

# CMake konfigurieren
cmake ..

# Kompilieren
make

# Optional: Installieren
sudo make install
```

### Windows (Visual Studio)

```bash
# Build-Verzeichnis erstellen
mkdir build
cd build

# CMake konfigurieren
cmake .. -G "Visual Studio 16 2019"

# Kompilieren
cmake --build . --config Release
```

### Windows (MinGW)

```bash
# Build-Verzeichnis erstellen
mkdir build
cd build

# CMake konfigurieren
cmake .. -G "MinGW Makefiles"

# Kompilieren
mingw32-make
```

## Ausführen

### Nach der Kompilierung

```bash
# Linux/macOS
./build/bin/OpenSylab

# Windows
.\build\bin\OpenSylab.exe
```

### Mit benutzerdefiniertem Datenbankpfad

```bash
# Als Kommandozeilenargument
./build/bin/OpenSylab /pfad/zur/datenbank.db

# Oder als Umgebungsvariable
export OPENSYLAB_DB_PATH=/pfad/zur/datenbank.db
./build/bin/OpenSylab
```

## Erste Schritte

1. **Programm starten**
   ```bash
   ./build/bin/OpenSylab
   ```

2. **Neue Probe erfassen**
   - Im Hauptmenü Option [1] wählen
   - Proben-ID eingeben (z.B. "S001")
   - Patienten-ID eingeben (z.B. "P12345")
   - Weitere Felder ausfüllen

3. **CSV-Import testen**
   - Im Hauptmenü Option [6] wählen
   - Pfad zur Beispiel-CSV angeben: `config/samples_example.csv`
   - Import bestätigen

4. **Proben anzeigen**
   - Im Hauptmenü Option [2] wählen
   - Alle importierten Proben werden aufgelistet

## Fehlerbehebung

### Problem: "sqlite3.h not found"

**Linux:**
```bash
sudo apt install libsqlite3-dev
```

**macOS:**
```bash
brew install sqlite3
```

**Windows:**
SQLite3-DLLs in das Projektverzeichnis kopieren oder Pfad in CMakeLists.txt anpassen.

### Problem: "CMake version too old"

```bash
# Ubuntu/Debian - CMake von Kitware PPA
sudo apt remove cmake
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt update
sudo apt install cmake
```

### Problem: Kompilierungsfehler in Visual Studio

- Sicherstellen, dass "Desktop development with C++" installiert ist
- C++17 Standard in Projekteigenschaften aktivieren
- SQLite3 als NuGet-Paket installieren: `Install-Package sqlite`

### Problem: Datenbank kann nicht erstellt werden

- Prüfen Sie Schreibrechte im aktuellen Verzeichnis
- Verwenden Sie einen absoluten Pfad: `./OpenSylab /tmp/opensylab.db`

## Build-Optionen

### Debug-Build

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### Release-Build (optimiert)

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

### Verbose Output

```bash
make VERBOSE=1
```

## Tests ausführen

```bash
cd build
make test
# Oder für detaillierte Ausgabe:
ctest --output-on-failure
# Oder direktes Ausführen:
./opensylab_tests
```

Die Test-Suite enthält 62 automatisierte Unit-Tests für alle Kernkomponenten.
Siehe [TESTING.md](TESTING.md) für Details.

## Deinstallation

```bash
# Wenn mit 'make install' installiert
sudo make uninstall

# Oder manuell
sudo rm /usr/local/bin/OpenSylab
sudo rm -rf /usr/local/share/OpenSylab
```

## Entwicklungsumgebung einrichten

### Visual Studio Code

1. Empfohlene Extensions installieren:
   - C/C++ (Microsoft)
   - CMake Tools
   - CMake

2. Workspace öffnen: `code .`

3. CMake konfigurieren: `Ctrl+Shift+P` → "CMake: Configure"

4. Build: `Ctrl+Shift+P` → "CMake: Build"

### CLion

1. Projekt öffnen: `File` → `Open` → OpenSylab-Verzeichnis auswählen

2. CMake wird automatisch konfiguriert

3. Build: `Build` → `Build Project` (Strg+F9)

## Weitere Hilfe

Bei Problemen:
1. Prüfen Sie die Systemanforderungen
2. Lesen Sie die Fehlermeldungen sorgfältig
3. Öffnen Sie ein Issue auf GitHub: https://github.com/yourusername/openSylab/issues

## Nächste Schritte

Nach erfolgreicher Installation siehe:
- [README.md](README.md) - Entwicklerdokumentation
- [../README.MD](../README.MD) - Projektübersicht und Features
