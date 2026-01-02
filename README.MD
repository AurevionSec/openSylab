# OpenSylab

## Einleitung & Projektzweck

OpenSylab ist ein Laborinformations- und Managementsystem (LIMS), das als Open-Source-Projekt ins Leben gerufen wurde, um eine entscheidende Lücke in der Laborwelt zu schließen. Viele kleine medizinische Labore stehen vor der Herausforderung, entweder auf teure proprietäre LIMS zurückzugreifen oder ineffiziente manuelle Prozesse zu nutzen. OpenSylab existiert, um dieser Problematik zu begegnen: Es bietet kostenfrei und quelloffen ein vollwertiges System, mit dem ein klinisches Diagnostiklabor seine gesamten Abläufe digital verwalten kann.

**Aktueller Entwicklungsstand:** Version 0.2.0 ist eine funktionsfähige Version mit CLI-Interface, die neben der Probenverwaltung nun auch Auftragsverwaltung, Ergebniseingabe mit Plausibilitätsprüfung, Gerätedatenschnittstelle (CSV/HL7), Audit-Trail und Benutzerauthentifizierung bietet. Web-Interface und REST-API sind für spätere Versionen geplant (siehe [ROADMAP.MD](ROADMAP.MD)).

## Manifest

Unser Manifest formuliert die grundlegenden Werte, Ziele und Prinzipien von OpenSylab:

- **Offenheit und Freiheit**: Der vollständige Quellcode ist und bleibt offen zugänglich
- **Gemeinschaft und Zusammenarbeit**: Weltweite Community-Entwicklung
- **Vertrauen und Transparenz**: Maximale Transparenz durch offenen Code
- **Qualität und Zuverlässigkeit**: Höchste Software-Qualität durch strenge Tests
- **Ethik und Gemeinwohl**: Gemeinnützig orientiert, kostenlos verfügbar
- **Dezentralisierung und Unabhängigkeit**: Daten bleiben vor Ort unter voller Kontrolle

## Features von OpenSylab 0.2.0

Die aktuelle Version bietet umfassende LIMS-Funktionalität:

### Implementierte Features

- **Probenerfassung und -verwaltung**:
  - Eindeutige Proben-IDs (Barcode-kompatibel)
  - Verknüpfung mit Patientendaten
  - Statusverfolgung (Erfasst, In Analyse, Analysiert, Validiert, Archiviert)
  - Vollständige CRUD-Operationen (Create, Read, Update, Delete)

- **Auftragsverwaltung** (NEU in v0.2):
  - Aufträge mit Status (REQUESTED, IN_PROGRESS, COMPLETED, VALIDATED, CANCELLED)
  - Prioritäten (NORMAL, URGENT, EMERGENCY)
  - Verknüpfung mit Proben

- **Ergebniseingabe** (NEU in v0.2):
  - Testergebnisse mit Status und Flags
  - Automatische Plausibilitätsprüfung gegen Referenzbereiche
  - Flag-Berechnung (NORMAL, LOW, HIGH, CRITICAL)

- **Gerätedatenschnittstelle** (NEU in v0.2):
  - CSV-Import für Geräteergebnisse
  - HL7-Parser für Laborgeräte
  - Batch-Import mit Validierung

- **Audit-Trail** (NEU in v0.2):
  - Vollständige Protokollierung aller Änderungen
  - Wer, Was, Wann für jede Aktion
  - Unveränderliche Audit-Einträge

- **Benutzerauthentifizierung** (NEU in v0.2):
  - Login/Logout-Funktionalität
  - Rollen: USER, ADMIN, LAB_TECH, VALIDATOR
  - Passwort-Hashing mit SHA-256
  - Benutzerverwaltung (nur für Admins)

- **Datenbankverwaltung**:
  - SQLite-Datenbank für zuverlässige Datenpersistenz
  - Transaktionssichere Speicherung
  - Fehlerbehandlung mit klarer Fehlerpropagierung

- **CSV-Import**:
  - Batch-Import von Proben aus CSV-Dateien
  - Pflichtfeldvalidierung (sample_id, patient_id)
  - Detaillierte Fehlerstatistik mit Zeilennummern
  - Whitespace-Erkennung und Ablehnung leerer Felder

- **Interaktive CLI**:
  - Menügeführte Benutzeroberfläche
  - Module: Proben, Aufträge, Ergebnisse, Benutzer
  - CSV-Import über Menü
  - Unicode-Symbole für bessere Lesbarkeit (✓ ✗ ℹ)

### In Entwicklung (zukünftige Versionen)

- Web-basierte Benutzeroberfläche
- REST-API für externe Systeme
- PostgreSQL-Unterstützung für größere Installationen
- Qualitätskontrolle (QC)
- Befunderstellung

Details siehe [ROADMAP.MD](ROADMAP.MD).

## Technologie-Stack (v0.2.0)

### Aktuelle Implementierung

- **Programmiersprache**: C++17
- **Build-System**: CMake 3.15+
- **Datenbank**: SQLite 3.x
- **Benutzeroberfläche**: Kommandozeilen-Interface (CLI)
- **Architektur**: Modulares Design mit klarer Trennung:
  - `core/` - Geschäftslogik und Datenmodelle
  - `db/` - Datenbankzugriffsschicht
  - `utils/` - CSV-Import und CLI-Interface

### Geplante Erweiterungen

- **Web-Framework**: drogon oder CppRestSDK für REST-API
- **Frontend**: React/Angular für Web-UI
- **Datenbank**: PostgreSQL für Multi-User-Betrieb
- **Sicherheit**: TLS/SSL, JWT-Authentifizierung, Verschlüsselung

## Projektstruktur (Repository-Struktur)

```text
OpenSylab/
├── src/                     # Quellcode der Anwendung
│   ├── core/                # Kernfunktionen (Sample-Klasse, Datenmodelle)
│   ├── db/                  # Datenbank-Zugriffsschicht (SQLite)
│   ├── utils/               # Hilfsfunktionen (CLI, CSV-Import)
│   └── main.cpp             # Hauptprogramm-Einstiegspunkt
├── include/                 # Header-Dateien
│   ├── core/                # Core-Headers
│   ├── db/                  # Database-Headers
│   └── utils/               # Utils-Headers
├── test/                    # Testfälle (62 Unit-Tests)
│   ├── CMakeLists.txt       # Test-Build-Konfiguration
│   └── unit/                # Unit-Tests
│       ├── test_runner.cpp  # Test-Framework
│       ├── test_sample.cpp  # Sample-Tests (6)
│       ├── test_database.cpp# Database-Tests (7)
│       └── test_csvimport.cpp# CSV-Import-Tests (5)
├── docs/                    # Dokumentation
│   ├── TESTING.md           # Test-Dokumentation
│   ├── INSTALL.md           # Installationsanleitung
│   └── README.md            # Entwickler-Dokumentation
├── config/                  # Beispiel-Konfigurationsdateien
│   ├── database.conf        # Datenbank-Konfiguration
│   └── samples_example.csv  # CSV-Beispieldaten
├── tools/                   # Hilfs-Skripte
├── CMakeLists.txt           # Build-Konfiguration (CMake)
├── build.sh                 # Build-Skript
├── test_and_build.sh        # Build & Test-Skript
├── LICENSE.txt              # Lizenz (nicht-kommerziell)
├── README.MD                # Dieses Dokument
├── ROADMAP.MD               # Entwicklungs-Roadmap
└── CHANGELOG.md             # Versionshistorie
```

## Installation und Build

### Voraussetzungen

- C++17-kompatibler Compiler (gcc 9+, clang 10+, MSVC 2019+)
- CMake 3.15+
- SQLite3 Development Libraries
  - **Linux**: `sudo apt-get install libsqlite3-dev`
  - **Fedora**: `sudo dnf install sqlite-devel`
  - **Windows**: SQLite wird automatisch gelinkt

### Kompilierung

```bash
# Repository klonen
cd openSylab

# Build mit Test-Skript (empfohlen)
chmod +x test_and_build.sh
./test_and_build.sh

# Oder manuell:
mkdir -p build && cd build
cmake ..
make

# Tests ausführen
./bin/opensylab_tests

# Programm starten
./bin/OpenSylab
```

Details siehe [docs/INSTALL.md](docs/INSTALL.md).

## Testing

OpenSylab v0.2.0 verfügt über ein selbst implementiertes Test-Framework mit **62 Unit-Tests**:

- **Sample-Tests**: Konstruktoren, Getter/Setter, Status-Konvertierung
- **Order-Tests**: Auftragsverwaltung, Status, Prioritäten
- **TestResult-Tests**: Ergebnisse, Plausibilitätsprüfung, Flags
- **Database-Tests**: CRUD-Operationen, Fehlerbehandlung
- **CSV-Import-Tests**: Proben- und Ergebnisimport, Validierung

```bash
# Tests ausführen
./build/bin/opensylab_tests

# Oder via CTest
cd build && ctest --output-on-failure
```

Detaillierte Informationen: [docs/TESTING.md](docs/TESTING.md)

**Test-Ergebnisse v0.2.0:**

```text
✓ Passed: 62
✗ Failed: 0
Total:   62
```

## Lizenz

OpenSylab wird unter einer eigenen offenen Lizenz veröffentlicht:

**Erlaubt:**

- Freie Verwendung für nicht-kommerzielle Zwecke
- Änderung und Weitergabe des Quellcodes
- Integration in nicht-kommerzielle Umgebungen

**Nicht erlaubt:**

- Kommerzielle Nutzung ohne Genehmigung
- Verkauf oder Unterlizenzierung
- Entfernung der Lizenzhinweise

Siehe [LICENSE.txt](LICENSE.txt) für Details.

## Roadmap

OpenSylab wird schrittweise entwickelt:

- **v0.1.1**: CLI + SQLite, Basis-Probenverwaltung ✓
- **v0.2.0** (Aktuell): Aufträge, Ergebnisse, Audit-Trail, Benutzer ✓
- **v0.5**: REST-API, Web-Interface (Alpha)
- **v1.0**: Produktionsreifes LIMS mit vollständigem Feature-Set
- **v2.0+**: Multi-Site, KI-Integration

Details siehe [ROADMAP.MD](ROADMAP.MD).

## Systemanforderungen (v0.2.0)

### Minimum

- **Betriebssystem**: Linux, Windows 10/11, macOS
- **Prozessor**: Dual-Core CPU
- **RAM**: 2 GB
- **Festplatte**: 100 MB (Software) + Datenbank-Speicher
- **Compiler**: C++17-fähig (zum Build)

### Empfohlen

- **Prozessor**: Quad-Core CPU
- **RAM**: 4 GB+
- **Festplatte**: SSD für bessere Performance
- **Netzwerk**: LAN (für zukünftige Multi-User-Features)

## Mitmachen & Community

OpenSylab lebt von seiner Community! Wir freuen uns über:

- **Code-Beiträge**: Pull Requests auf GitHub
- **Feedback & Testing**: Bugreports und Feature-Requests
- **Dokumentation**: Verbesserungen, Übersetzungen
- **Fachliche Beratung**: Input von Laborfachkräften

### Beitragen

1. Repository forken
2. Branch erstellen (`git checkout -b feature/mein-feature`)
3. Änderungen committen
4. Tests ausführen (`./test_and_build.sh`)
5. Pull Request stellen

Siehe [CONTRIBUTING.md](CONTRIBUTING.md) für Details.

### Qualitätssicherung

- Automatisierte Tests (CI/CD)
- Code Reviews für alle Pull Requests
- Strikte Coding-Standards
- Regelmäßige Sicherheitsaudits

## Kontakt & Support

- **GitHub**: [OpenSylab Repository](https://github.com/opensylab/opensylab)
- **Issues**: [GitHub Issues](https://github.com/opensylab/opensylab/issues)
- **Diskussionen**: [GitHub Discussions](https://github.com/opensylab/opensylab/discussions)
- **E-Mail**: A@Eddelbuet.tel

## Danksagung

OpenSylab wird entwickelt von einer weltweiten Community von Entwicklern, Laborfachleuten und Enthusiasten. Danke an alle Beitragenden!

## Versionshistorie

Siehe [CHANGELOG.md](CHANGELOG.md) für detaillierte Änderungen.

**Aktuelle Version**: v0.2.0 (2026-01-02)

- ✓ 62 Unit-Tests implementiert
- ✓ Auftragsverwaltung (Order-Modul)
- ✓ Ergebniseingabe mit Plausibilitätsprüfung
- ✓ Gerätedatenschnittstelle (CSV/HL7)
- ✓ Audit-Trail für alle Änderungen
- ✓ Benutzerauthentifizierung und Rollenverwaltung

---

*OpenSylab - Open-Source LIMS für Labore weltweit. Kostenlos, transparent, vertrauenswürdig.*
