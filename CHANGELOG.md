# OpenSylab - Changelog

Alle wichtigen Änderungen an diesem Projekt werden in dieser Datei dokumentiert.

## [0.2.0] - 2026-01-02

### Neue Features

#### 🆕 Auftragsverwaltung (Order-Modul)
- **Order-Datenmodell** mit Status-Workflow:
  - Status: REQUESTED → IN_PROGRESS → COMPLETED → VALIDATED → CANCELLED
  - Priorität: NORMAL, URGENT, EMERGENCY
  - Verknüpfung zu Proben via sampleId
- **CRUD-Operationen** für Orders in Database
- **CLI-Integration**: Menüpunkte 20-26 für Order-Verwaltung
- **Dateien**:
  - `include/core/Order.h` - Order-Datenmodell
  - `src/core/Order.cpp` - Implementierung
  - `test/unit/test_order.cpp` - Unit-Tests (8 Tests)

#### 🆕 Ergebniseingabe (TestResult-Modul)
- **TestResult-Datenmodell** mit Validierungs-Workflow:
  - Status: PENDING → REVIEWED → VALIDATED → REJECTED → AMENDED
  - Flags: NORMAL, ABNORMAL, CRITICAL, INCONCLUSIVE
  - Referenzbereiche (minValue, maxValue) mit automatischer Flag-Berechnung
- **CRUD-Operationen** für TestResults in Database
- **CLI-Integration**: Menüpunkte 30-36 für Ergebnis-Verwaltung
- **Dateien**:
  - `include/core/TestResult.h` - TestResult-Datenmodell
  - `src/core/TestResult.cpp` - Implementierung
  - `test/unit/test_testresult.cpp` - Unit-Tests (10 Tests)

#### 🆕 Gerätedatenschnittstelle (CSV-Ergebnisimport)
- **CsvResultImport-Klasse** für Laborgerätedaten:
  - Import von Analysegeräte-Ergebnissen im CSV-Format
  - Automatische Flag-Berechnung basierend auf Referenzbereichen
  - Verknüpfung mit bestehenden Orders
- **Format**: `order_id,parameter,value,unit,min_value,max_value`
- **Fehlertolerantes Parsing** mit detaillierter Statistik
- **Dateien**:
  - `include/utils/CsvResultImport.h` - Header
  - `src/utils/CsvResultImport.cpp` - Implementierung
  - `test/unit/test_csvresultimport.cpp` - Unit-Tests (5 Tests)

#### 🆕 Audit-Trail (rudimentär)
- **AuditEntry-Datenmodell** für lückenlose Protokollierung:
  - EntityType: SAMPLE, ORDER, RESULT, USER, SYSTEM
  - ActionType: CREATE, UPDATE, DELETE, VIEW, VALIDATE, LOGIN, LOGOUT
  - Zeitstempel, Benutzer, Details
- **Automatisches Logging** bei allen CRUD-Operationen
- **CLI-Integration**: Menüpunkte 50-51 für Audit-Anzeige
- **Dateien**:
  - `include/core/AuditEntry.h` - AuditEntry-Datenmodell
  - `src/core/AuditEntry.cpp` - Implementierung

#### 🆕 Benutzer-Authentifizierung
- **User-Datenmodell** mit Rollen-System:
  - Rollen: ADMIN, OPERATOR, VIEWER
  - Aktiv/Inaktiv-Status
  - Passwort-Hashing (DJB2 mit Salt)
- **Authentifizierung** mit Login/Logout
- **Berechtigungsprüfung** im CLI:
  - Admin: Vollzugriff inkl. Benutzerverwaltung
  - Operator: Erstellen, Bearbeiten, Löschen
  - Viewer: Nur Lesezugriff
- **CLI-Integration**: Menüpunkte 40-46 für Benutzerverwaltung
- **Dateien**:
  - `include/core/User.h` - User-Datenmodell
  - `src/core/User.cpp` - Implementierung

### Kritische Bugfixes

#### 🔴 HIGH - SQLite Foreign Key Enforcement
- **Problem**: SQLite Foreign Keys waren definiert aber nicht aktiviert
- **Lösung**: `PRAGMA foreign_keys = ON` nach Datenbankverbindung
- **Datei**: `src/db/Database.cpp`

### Verbesserungen

- **Test-Suite erweitert**: Von 18 auf 62 Tests
- **CLI um 26 neue Menüpunkte** erweitert
- **Datenbank-Schema** um 4 neue Tabellen erweitert (orders, test_results, audit_log, users)
- **Namespace-Struktur** beibehalten (opensylab::core, opensylab::db, opensylab::utils)

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
