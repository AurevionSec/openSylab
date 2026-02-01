# OpenSylab v0.2 - Entwicklerdokumentation

## Überblick

OpenSylab v0.2 ist eine funktionsfähige Version des Open Source Laboratory Information Management Systems (LIMS). Diese Version implementiert Auftragsverwaltung, Ergebniseingabe, Gerätedatenschnittstelle, Audit-Trail und Benutzerauthentifizierung.

## Architektur

### Modulstruktur

```
OpenSylab/
├── src/
│   ├── core/              # Kernlogik und Datenmodelle
│   │   ├── Sample.cpp      # Proben-Datenmodell
│   │   ├── Order.cpp       # Auftrags-Datenmodell
│   │   ├── TestResult.cpp  # Ergebnis-Datenmodell
│   │   ├── AuditEntry.cpp  # Audit-Protokoll
│   │   └── User.cpp        # Benutzer-Datenmodell
│   ├── db/                # Datenbank-Zugriffsschicht
│   │   └── Database.cpp    # SQLite-Implementierung
│   ├── utils/             # Hilfsfunktionen
│   │   ├── CsvImport.cpp       # CSV-Proben-Import
│   │   ├── CsvResultImport.cpp # CSV-Ergebnis-Import
│   │   └── CliInterface.cpp    # Kommandozeile
│   └── main.cpp           # Programmeinstieg
├── include/               # Header-Dateien
│   ├── core/
│   ├── db/
│   └── utils/
├── test/                  # Unit-Tests (62 Tests)
├── docs/                  # Dokumentation
└── config/                # Konfigurationsdateien
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

#### 3. Order (Auftrags-Datenmodell)
- Repräsentiert einen Laborauftrag
- Eigenschaften: orderId, sampleId, testType, status, priority, requestedBy, notes
- Status-Workflow: REQUESTED → IN_PROGRESS → COMPLETED → VALIDATED → CANCELLED
- Prioritäten: NORMAL, URGENT, EMERGENCY
- Verknüpfung zu Proben via sampleId

#### 4. TestResult (Ergebnis-Datenmodell)
- Repräsentiert ein Analyseergebnis
- Eigenschaften: resultId, orderId, parameter, value, unit, flag, status
- Status-Workflow: PENDING → REVIEWED → VALIDATED → REJECTED → AMENDED
- Flags: NORMAL, ABNORMAL, CRITICAL, INCONCLUSIVE
- Referenzbereiche (minValue, maxValue) mit automatischer Flag-Berechnung

#### 5. AuditEntry (Audit-Protokoll)
- Lückenlose Protokollierung aller Änderungen
- EntityType: SAMPLE, ORDER, RESULT, USER, SYSTEM
- ActionType: CREATE, UPDATE, DELETE, VIEW, VALIDATE, LOGIN, LOGOUT
- Zeitstempel, Benutzer, Details

#### 6. User (Benutzer-Datenmodell)
- Benutzerverwaltung mit Rollen
- Rollen: ADMIN, OPERATOR, VIEWER
- Passwort-Hashing (DJB2 mit Salt)
- Aktiv/Inaktiv-Status

#### 7. CsvImport (CSV-Import)
- Import von Probendaten aus CSV-Dateien
- Konfigurierbares Trennzeichen
- Fehlertolerantes Parsing
- Format: `sample_id,patient_id,patient_name,description,status`

#### 8. CsvResultImport (CSV-Ergebnis-Import)
- Import von Analysegeräte-Ergebnissen
- Automatische Flag-Berechnung basierend auf Referenzbereichen
- Format: `order_id,parameter,value,unit,min_value,max_value`

#### 9. CliInterface (Command-Line Interface)
- Interaktives Menüsystem
- Vollständige CRUD-Operationen für Samples, Orders, TestResults, Users
- CSV-Import-Funktionen
- Audit-Log-Anzeige
- Login/Logout mit Berechtigungsprüfung
- Statistik-Übersicht

## Implementierte Features (v0.2)

✅ **Grundlegende Architektur**
- Modulare C++-Struktur
- CMake Build-System
- Namespace-Organisation

✅ **Probenverwaltung**
- Probe erfassen, anzeigen, suchen, aktualisieren, löschen
- Status-Workflow: Erfasst → In Analyse → Analysiert → Validiert → Archiviert

✅ **Auftragsverwaltung** (NEU in v0.2)
- Auftrag erstellen, anzeigen, suchen, aktualisieren, löschen
- Status-Workflow: Angefordert → In Bearbeitung → Abgeschlossen → Validiert
- Prioritäten: Normal, Dringend, Notfall
- Verknüpfung mit Proben

✅ **Ergebniseingabe** (NEU in v0.2)
- Ergebnis erfassen, anzeigen, validieren
- Automatische Flag-Berechnung (Normal, Abnormal, Kritisch)
- Referenzbereiche
- Status-Workflow mit Validierung

✅ **Gerätedatenschnittstelle** (NEU in v0.2)
- CSV-Import von Analysegeräte-Ergebnissen
- Automatische Verknüpfung mit Aufträgen
- Fehlertolerantes Parsing

✅ **Audit-Trail** (NEU in v0.2)
- Lückenlose Protokollierung aller Änderungen
- Benutzer, Zeitstempel, Details
- Filterung nach Entität

✅ **Benutzerauthentifizierung** (NEU in v0.2)
- Login/Logout
- Rollen: Admin, Operator, Viewer
- Passwort-Hashing
- Berechtigungsprüfung

## Sicherheit & Compliance (Policies)

### API-Key-Lebenszyklus (Doc-Policy)
- API-Keys werden in der Datenbank verwaltet und sind aktiv/inaktiv schaltbar.
- Rotation: Neuer Key wird erstellt, alter Key wird deaktiviert (kein Re-Use).
- Revocation: Key deaktivieren und aus aktiven Systemen entfernen.
- Keys niemals im Klartext teilen; Änderungen sind zu auditieren.

### Support-Datenminimierung (Support Role)
- Support darf Listen/Detailansichten sehen, aber nur reduzierte Felder.
- Sichtbar: IDs, Status, Zeitstempel, Testtyp, Priorität, Flags.
- Versteckt: Patientenname, Notizen, Kommentare, Referenzbereiche, measured_by.
- Ergebniswerte/Einheit werden in Support-Ansichten ausgeblendet.
- Zugriff wird beim ersten Anzeigen protokolliert (Auto-Refresh erzeugt keine neuen Logs).

### Auto-Refresh & Systemlast
- Auto-Refresh ist opt-in (Standard: aus).
- Mehrere parallele Nutzer mit Auto-Refresh erhöhen die DB-Last.
- Empfehlung: nur bei Bedarf aktivieren und Intervall stabil halten.

### Audit-Log-Integrität (Assessment)
- Aktuell kein hash-chain / WORM-Speicher implementiert.
- Für strengere Compliance: Hash-Ketten, Append-only Storage oder externe Audit-Sinks prüfen.

✅ **Datenbank**
- SQLite-Integration mit Foreign Key Enforcement
- CRUD-Operationen für alle Entitäten
- Automatische Schema-Erstellung
- Indizes für Performance

✅ **Import/Export**
- CSV-Import von Probendaten
- CSV-Import von Analyseergebnissen
- Fehlerbehandlung beim Import

✅ **Benutzeroberfläche**
- CLI mit interaktivem Menü (50+ Menüpunkte)
- Eingabevalidierung
- Rollenbasierte Menüanzeige
- Statistik-Anzeige

✅ **Tests**
- 62 automatisierte Unit-Tests
- Eigenes Test-Framework ohne externe Abhängigkeiten

## Verwendete Technologien

- **Programmiersprache**: C++17
- **Build-System**: CMake 3.15+
- **Datenbank**: SQLite3
- **Standard Library**: STL (Standard Template Library)

## Bekannte Einschränkungen (v0.2)

- Keine Netzwerkfähigkeit (nur lokale Datenbank)
- Keine Web-Oberfläche (nur CLI)
- Einfaches Passwort-Hashing (nicht PBKDF2/bcrypt)
- Keine Sitzungsverwaltung (Login nur pro Programmlauf)
- Keine Datenexport-Funktionen (nur Import)
- Keine HL7/FHIR-Schnittstellen

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

## Nächste Schritte (für v0.3)

- [ ] Sichere Passwort-Speicherung (PBKDF2 oder bcrypt)
- [ ] Sitzungsverwaltung mit Timeout
- [ ] Datenexport (CSV, PDF-Reports)
- [ ] HL7-Schnittstelle (rudimentär)
- [ ] Logging-System (Datei-basiert)
- [ ] Batch-Verarbeitung für Hochdurchsatz
- [ ] Performance-Optimierungen

## Referenzen

Siehe auch:
- [INSTALL.md](INSTALL.md) - Installations- und Kompilierungsanleitung
- [../README.MD](../README.MD) - Projekt-Übersicht
- [../ROADMAP.MD](../ROADMAP.MD) - Entwicklungs-Roadmap
