# OpenSylab

## Einleitung & Projektzweck

OpenSylab ist ein Laborinformations- und Managementsystem (LIMS), das als Open-Source-Projekt ins Leben gerufen wurde, um eine entscheidende Lücke in der Laborwelt zu schließen. Viele kleine medizinische Labore stehen vor der Herausforderung, entweder auf teure proprietäre LIMS zurückzugreifen oder ineffiziente manuelle Prozesse zu nutzen. OpenSylab existiert, um dieser Problematik zu begegnen: Es bietet kostenfrei und quelloffen ein vollwertiges System, mit dem ein klinisches Diagnostiklabor seine gesamten Abläufe digital verwalten kann.

**Aktueller Entwicklungsstand:** Version 0.5.0 ist eine funktionsfähige Version mit CLI-Interface UND moderner React-Weboberfläche. Das System bietet vollständige CRUD-Operationen über REST-API, JWT-basierte Authentifizierung, TLS/HTTPS-Verschlüsselung, Probenverwaltung mit Web-UI, Auftragsverwaltung, Ergebniseingabe mit Plausibilitätsprüfung, Gerätedatenschnittstelle (CSV/HL7), Audit-Trail und Benutzerauthentifizierung. (siehe [ROADMAP.MD](ROADMAP.MD)).

## Manifest

Unser Manifest formuliert die grundlegenden Werte, Ziele und Prinzipien von OpenSylab:

- **Offenheit und Freiheit**: Der vollständige Quellcode ist und bleibt offen zugänglich
- **Gemeinschaft und Zusammenarbeit**: Weltweite Community-Entwicklung
- **Vertrauen und Transparenz**: Maximale Transparenz durch offenen Code
- **Qualität und Zuverlässigkeit**: Höchste Software-Qualität durch strenge Tests
- **Ethik und Gemeinwohl**: Gemeinnützig orientiert, kostenlos verfügbar
- **Dezentralisierung und Unabhängigkeit**: Daten bleiben vor Ort unter voller Kontrolle

## Features von OpenSylab 0.5.0

Die aktuelle Version bietet umfassende LIMS-Funktionalität mit moderner Webtechnologie:

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

- **JWT-Authentifizierung** (NEU in v0.5):
  - Token-basierte Authentifizierung (JWT mit HS256)
  - Login-Endpoint mit Username/Password
  - 60-Minuten Token-Gültigkeit
  - Automatische Token-Validierung und -Erneuerung
  - Rückwärtskompatibilität mit API-Key

- **React Web-Interface** (NEU in v0.5):
  - Moderne Single Page Application (SPA)
  - Responsive Design mit TailwindCSS
  - Dashboard mit Übersicht
  - **Probenverwaltung** (Samples): Liste, Filtering, Create/Edit Modals
  - **Auftragsverwaltung** (Orders): Liste, Dual-Filtering (Status + Priority), Create/Edit
  - **Ergebnisverwaltung** (Results): Liste, Dual-Filtering (Status + Flag), Anzeige mit Badges
  - Protected Routes mit JWT-Auth
  - Real-time Formularvalidierung

- **REST-API** (NEU in v0.5):
  - Vollständige CRUD-Operationen (GET, POST, PUT, DELETE)
  - Endpoints: /api/v1/samples, /api/v1/orders, /api/v1/results
  - JSON-basierte Request/Response
  - CORS-Unterstützung für Frontend
  - Konsistente Fehlerbehandlung
  - Audit-Logging für alle Operationen

- **TLS/HTTPS-Unterstützung** (NEU in v0.5):
  - OpenSSL-Integration für verschlüsselte Verbindungen
  - TLS 1.2+ mit modernen Cipher Suites
  - Optionaler HTTPS-Modus
  - Zertifikatsverwaltung

- **Benutzerauthentifizierung**:
  - Login/Logout-Funktionalität (CLI und Web)
  - Rollen: USER, ADMIN, LAB_TECH, VALIDATOR
  - Passwort-Hashing mit DJB2 (Salt)
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

- Dashboard mit Statistiken und Charts
- User Management UI (Admin Panel)
- PostgreSQL-Unterstützung für größere Installationen
- Qualitätskontrolle (QC)
- Befunderstellung mit PDF-Export
- Docker-Containerisierung
- Multi-Tenant-Fähigkeit

Details siehe [ROADMAP.MD](ROADMAP.MD).

## Technologie-Stack (v0.5.0)

### Backend

- **Programmiersprache**: C++17
- **Build-System**: CMake 3.15+
- **Datenbank**: SQLite 3.x
- **REST-API**: Custom HTTP-Server mit JSON-Support
- **Authentifizierung**: JWT (jwt-cpp v0.7.0)
- **Verschlüsselung**: OpenSSL 3.x für TLS/HTTPS
- **Architektur**: Modulares Design mit klarer Trennung:
  - `core/` - Geschäftslogik und Datenmodelle
  - `db/` - Datenbankzugriffsschicht
  - `api/` - REST-API und HTTP-Server
  - `auth/` - JWT-Authentifizierung
  - `utils/` - CSV-Import und CLI-Interface

### Frontend

- **Framework**: React 18 mit TypeScript
- **Build-Tool**: Vite mit Hot Module Replacement
- **Styling**: TailwindCSS für responsive UI
- **Routing**: React Router für Single Page Application
- **HTTP-Client**: Axios mit JWT-Interceptors
- **State Management**: React Context API

### Geplante Erweiterungen

- **Datenbank**: PostgreSQL für Multi-User-Betrieb
- **Containerisierung**: Docker und Docker Compose
- **Testing**: Jest für Frontend, erweiterte Backend-Tests

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
  - **Linux**: `sudo apt-get install libsqlite3-dev libssl-dev`
  - **Fedora**: `sudo dnf install sqlite-devel openssl-devel`
- Node.js 18+ und npm (für Frontend)
- OpenSSL 3.x Development Libraries (für TLS/HTTPS)
  - **Windows**: SQLite wird automatisch gelinkt

### Kompilierung

```bash
# Repository klonen
cd openSylab

# Backend bauen
mkdir -p build && cd build
cmake ..
make

# Backend starten
./bin/OpenSylab --api-port 8080

# Frontend bauen (in neuem Terminal)
cd frontend
npm install
npm run dev

# Browser öffnen: http://localhost:5173
# API läuft auf: http://localhost:8080/api/v1

# Tests ausführen
./build/bin/opensylab_tests
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
- **v0.2.0**: Aufträge, Ergebnisse, Audit-Trail, Benutzer ✓
- **v0.5.0** (Aktuell): REST-API, React Web-UI, JWT-Auth, TLS/HTTPS ✓
- **v0.6**: Docker, erweiterte Web-UI Features
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

**Aktuelle Version**: v0.5.0 (2026-02-01)

- ✓ React Web-Interface mit TypeScript
- ✓ REST-API mit vollständigen CRUD-Operationen (GET, POST, PUT, DELETE)
- ✓ JWT-basierte Authentifizierung
- ✓ TLS/HTTPS-Verschlüsselung
- ✓ Samples Management Web-UI (Create, Edit, List, Filter)
- ✓ Orders Management Web-UI (Create, Edit, List, Dual-Filter)
- ✓ Results Management Web-UI (List, Dual-Filter, Badge-Display)
- ✓ Dashboard und geschützte Routen
- ✓ 62 Unit-Tests (Backend)
- ✓ Auftragsverwaltung, Ergebniseingabe, Audit-Trail (Backend + Frontend)

---

*OpenSylab - Open-Source LIMS für Labore weltweit. Kostenlos, transparent, vertrauenswürdig.*
