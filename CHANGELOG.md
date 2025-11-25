# OpenSylab - Changelog

Alle wichtigen Änderungen an diesem Projekt werden in dieser Datei dokumentiert.

## [0.1.1] - 2025-11-25

### Kritische Bugfixes

#### 🔴 HIGH - Automatisierte Tests hinzugefügt
- **Problem**: Keine automatisierten Tests vorhanden; fehlender Regressionsschutz
- **Lösung**:
  - Einfaches Test-Framework ohne externe Abhängigkeiten implementiert
  - Unit-Tests für Sample-Klasse (6 Tests)
  - Unit-Tests für Database-Klasse (7 Tests)
  - Unit-Tests für CsvImport-Klasse (5 Tests)
  - Test-Runner mit farbiger Ausgabe
  - Gesamt: 18 automatisierte Tests
- **Dateien**:
  - `test/CMakeLists.txt` - Test-Konfiguration
  - `test/unit/test_runner.cpp` - Test-Framework
  - `test/unit/test_sample.cpp` - Sample-Tests
  - `test/unit/test_database.cpp` - Database-Tests
  - `test/unit/test_csvimport.cpp` - CSV-Import-Tests
  - `test_and_build.sh` - Build & Test-Skript

#### 🟡 MEDIUM - Eingabevalidierung in CLI
- **Problem**: CLI akzeptiert leere/whitespace-belegte IDs; Datenbank enthält unbrauchbare Datensätze
- **Lösung**:
  - `readValidatedInput()` Funktion mit Pflichtfeldprüfung
  - `trim()` Funktion entfernt führende/nachfolgende Whitespaces
  - `isValidId()` prüft auf gültige Zeichen (alphanumerisch, -, _)
  - `isEmpty()` erkennt leere/whitespace-Strings
  - Benutzerfreundliche Fehlermeldungen mit Wiederholungsschleife
- **Dateien**:
  - `include/utils/CliInterface.h` - Neue Validierungsmethoden
  - `src/utils/CliInterface.cpp` - Implementierung

#### 🟡 MEDIUM - CSV-Import Pflichtfeldprüfung
- **Problem**: CSV-Import akzeptiert Zeilen ohne Pflichtfelder; signalisiert Fehler nur über STDERR
- **Lösung**:
  - Pflichtfeldvalidierung für `sample_id` und `patient_id`
  - Whitespace-Prüfung mit `std::invalid_argument` Exception
  - Detaillierte Fehlerstatistik (✓ Erfolgreich / ✗ Fehler)
  - Klare Unterscheidung zwischen "keine Daten" und "Fehler aufgetreten"
  - Fehlerbehandlung mit Zeilennummern
- **Dateien**:
  - `src/utils/CsvImport.cpp` - Verbesserte Validierung

#### 🟡 MEDIUM - Database::getAllSamples Fehlerbehandlung
- **Problem**: Bei SQL-Fehlern wird leerer Vektor zurückgegeben; CLI meldet fälschlich "Keine Proben"
- **Lösung**:
  - `hasError()` Methode zur Fehlererkennung
  - `clearError()` Methode zum Zurücksetzen des Fehlerzustands
  - CLI prüft nun `hasError()` vor Anzeige
  - Unterscheidung zwischen leerem Ergebnis und Fehler
  - Exception-Handling beim Iterieren über Ergebnisse
- **Dateien**:
  - `include/db/Database.h` - Neue Fehlerbehandlungsmethoden
  - `src/db/Database.cpp` - Verbesserte getAllSamples()
  - `src/utils/CliInterface.cpp` - Fehlerprüfung in handleListSamples() und handleStatistics()

### Verbesserte Benutzerfreundlichkeit
- Konsistente Unicode-Symbole (✓ ✗ ℹ) für bessere Lesbarkeit
- Klare Fehler- und Erfolgsmeldungen in allen Modulen
- Verbesserte Ausgabe mit Formatierung

### Entwickler-Tools
- Neues Skript: `test_and_build.sh` - Kompiliert und testet in einem Schritt
- Aktualisiertes `build.sh` - Vereinfachter Build-Prozess

## [0.1.0] - 2025-11-24

### Initial Release
- Grundlegende Projektstruktur
- C++17-basierte Implementierung
- SQLite-Datenbank-Integration
- CLI-Interface
- CSV-Import-Funktion
- Probenverwaltung (CRUD)
- Modulare Architektur
- CMake Build-System
- Basis-Dokumentation
