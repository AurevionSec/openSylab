#!/bin/bash
#
# OpenSylab v0.1 - Build-Skript
# Dieses Skript kompiliert das OpenSylab-Projekt
#

set -e  # Bei Fehler abbrechen

echo "================================"
echo "OpenSylab v0.1 Build-Skript"
echo "================================"
echo ""

# In Projektverzeichnis wechseln
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Projektverzeichnis: $SCRIPT_DIR"
echo ""

# Build-Verzeichnis erstellen
if [ -d "build" ]; then
    echo "Lösche altes Build-Verzeichnis..."
    rm -rf build
fi

echo "Erstelle Build-Verzeichnis..."
mkdir -p build
cd build

echo ""
echo "Konfiguriere Projekt mit CMake..."
echo "================================"
cmake ..

echo ""
echo "Kompiliere Projekt..."
echo "================================"
make

echo ""
echo "================================"
echo "Build erfolgreich!"
echo "================================"
echo ""
echo "Das Programm wurde erstellt in: build/bin/OpenSylab"
echo ""
echo "Zum Ausführen:"
echo "  ./build/bin/OpenSylab"
echo ""
echo "Oder mit benutzerdefinierter Datenbank:"
echo "  ./build/bin/OpenSylab /pfad/zur/datenbank.db"
echo ""
