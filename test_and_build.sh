#!/bin/bash
#
# OpenSylab v0.1 - Build und Test-Skript
# Kompiliert das Projekt und führt alle Tests aus
#

set -e  # Bei Fehler abbrechen

echo "═══════════════════════════════════════════════════════════"
echo "  OpenSylab v0.1 - Build & Test Script"
echo "═══════════════════════════════════════════════════════════"
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
echo "═══════════════════════════════════════════════════════════"
echo "  SCHRITT 1: CMake Konfiguration"
echo "═══════════════════════════════════════════════════════════"
cmake ..

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  SCHRITT 2: Kompilierung"
echo "═══════════════════════════════════════════════════════════"
make

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  SCHRITT 3: Unit-Tests ausführen"
echo "═══════════════════════════════════════════════════════════"
make test || ctest --output-on-failure

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  ✓ Build und Tests erfolgreich!"
echo "═══════════════════════════════════════════════════════════"
echo ""
echo "Binaries:"
echo "  - Hauptprogramm: ./build/bin/OpenSylab"
echo "  - Tests:         ./build/opensylab_tests"
echo ""
echo "Zum Ausführen:"
echo "  ./build/bin/OpenSylab"
echo ""
