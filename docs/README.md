# OpenSylab v0.1 - Entwicklerdokumentation

## Überblick

OpenSylab v0.1 ist die erste Proof-of-Concept-Version des Open Source Laboratory Information Management Systems (LIMS). Diese Version implementiert die grundlegende Architektur und Kernfunktionalität.

## Architektur

### Modulstruktur

```
OpenSylab/
├── src/
│   ├── core/          # Kernlogik und Datenmodelle
│   │   └── Sample.cpp  # Proben-Datenmodell
│   ├── db/            # Datenbank-Zugriffsschicht
│   │   └── Database.cpp # SQLite-Implementierung
│   ├── utils/         # Hilfsfunktionen
│   │   ├── CsvImport.cpp
│   │   └── CliInterface.cpp
│   └── main.cpp       # Programmeinstieg
├── include/           # Header-Dateien
│   ├── core/
│   ├── db/
│   └── utils/
├── test/              # Tests (für zukünftige Versionen)
├── docs/              # Dokumentation
└── config/            # Konfigurationsdateien
```

### Komponenten

#### 1. Sample (Proben-Datenmodell)
- Repräsentiert eine Laborprobe im System
- Eigenschaften: ID, Barcode, Patient-ID, Name, Status, etc.
- Status-Management: Erfasst → In Analyse → Analysiert → Validiert → Archiviert

#### 2. Database (Datenbank-Zugriffsschicht)
- SQLite-basierte Persistierung
- CRUD-Operationen für Samples
- Automatische Schema-Initialisierung
- Fehlerbehandlung und Logging

#### 3. CsvImport (CSV-Import)
- Import von Probendaten aus CSV-Dateien
- Konfigurierbares Trennzeichen
- Fehlertolerantes Parsing
- Format: `sample_id,patient_id,patient_name,description,status`

#### 4. CliInterface (Command-Line Interface)
- Interaktives Menüsystem
- Vollständige CRUD-Operationen
- CSV-Import-Funktion
- Statistik-Übersicht

## Implementierte Features (v0.1)

✅ **Grundlegende Architektur**
- Modulare C++-Struktur
- CMake Build-System
- Namespace-Organisation

✅ **Probenverwaltung**
- Probe erfassen
- Proben anzeigen
- Probe suchen (nach Barcode)
- Probe aktualisieren (Status ändern)
- Probe löschen

✅ **Datenbank**
- SQLite-Integration
- CRUD-Operationen
- Automatische Schema-Erstellung
- Indizes für Performance

✅ **Import/Export**
- CSV-Import von Probendaten
- Fehlerbehandlung beim Import

✅ **Benutzeroberfläche**
- CLI mit interaktivem Menü
- Eingabevalidierung
- Statistik-Anzeige

## Verwendete Technologien

- **Programmiersprache**: C++17
- **Build-System**: CMake 3.15+
- **Datenbank**: SQLite3
- **Standard Library**: STL (Standard Template Library)

## Bekannte Einschränkungen (v0.1)

- Keine Benutzerauthentifizierung
- Keine Rechteverwaltung
- Kein Audit-Trail
- Keine Geräteschnittstellen
- Keine Netzwerkfähigkeit
- Keine Web-Oberfläche
- Keine automatisierten Tests

Diese Features sind für spätere Versionen geplant (siehe ROADMAP.MD).

## Code-Konventionen

### Namenskonventionen
- Klassen: PascalCase (z.B. `Sample`, `Database`)
- Methoden: camelCase (z.B. `getSampleId()`, `createSample()`)
- Member-Variablen: camelCase mit Trailing-Underscore (z.B. `sampleId_`)
- Konstanten: UPPER_CASE (z.B. `DEFAULT_DB_PATH`)

### Namespace-Struktur
```cpp
opensylab::core     // Kernlogik
opensylab::db       // Datenbank
opensylab::utils    // Hilfsfunktionen
```

### Fehlerbehandlung
- Rückgabe von `bool` für Erfolg/Fehler
- `getLastError()` für Fehlerdetails
- Exceptions nur für kritische Fehler

## Erweiterungsmöglichkeiten

### Für Entwickler

1. **Neue Datenmodelle hinzufügen**
   - Header in `include/core/`
   - Implementierung in `src/core/`
   - Datenbank-Schema in `Database::initializeSchema()`

2. **Neue CLI-Befehle**
   - Methode in `CliInterface.h/cpp` hinzufügen
   - Im Hauptmenü registrieren

3. **Neue Import-Formate**
   - Neue Klasse nach Vorbild von `CsvImport`
   - Im CLI-Menü integrieren

## Nächste Schritte (für v0.2)

- [ ] Test-Framework integrieren (Google Test)
- [ ] Unit-Tests für alle Komponenten
- [ ] Auftragsverwaltung hinzufügen
- [ ] Ergebniseingabe implementieren
- [ ] Einfache Gerätedatenschnittstelle (CSV)
- [ ] Logging-System

## Referenzen

Siehe auch:
- [INSTALL.md](INSTALL.md) - Installations- und Kompilierungsanleitung
- [../README.MD](../README.MD) - Projekt-Übersicht
- [../ROADMAP.MD](../ROADMAP.MD) - Entwicklungs-Roadmap
