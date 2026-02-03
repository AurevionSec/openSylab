# OpenSylab

## Einleitung & Projektzweck

OpenSylab ist ein Laborinformations- und Managementsystem (LIMS), das als Open-Source-Projekt ins Leben gerufen wurde, um eine entscheidende Lücke in der Laborwelt zu schließen. Viele kleine medizinische Labore stehen vor der Herausforderung, entweder auf teure proprietäre LIMS zurückzugreifen oder ineffiziente manuelle Prozesse zu nutzen. OpenSylab existiert, um dieser Problematik zu begegnen: Es bietet kostenfrei und quelloffen ein vollwertiges System, mit dem ein klinisches Diagnostiklabor seine gesamten Abläufe digital verwalten kann.

**Aktueller Entwicklungsstand:** Version 0.6.0 ist eine produktionsnahe Version mit vollständigem LIMS-Frontend. Das System bietet vollständige CRUD-Operationen über REST-API, JWT-basierte Authentifizierung mit Role-Based Access Control (RBAC), TLS/HTTPS-Verschlüsselung, User Management System, Audit Log für Compliance, User Profile Management, Enhanced Dashboard mit Statistiken, und professionelle React-Weboberfläche mit TypeScript (siehe [ROADMAP.MD](ROADMAP.MD)).

## Manifest

Unser Manifest formuliert die grundlegenden Werte, Ziele und Prinzipien von OpenSylab:

- **Offenheit und Freiheit**: Der vollständige Quellcode ist und bleibt offen zugänglich
- **Gemeinschaft und Zusammenarbeit**: Weltweite Community-Entwicklung
- **Vertrauen und Transparenz**: Maximale Transparenz durch offenen Code
- **Qualität und Zuverlässigkeit**: Höchste Software-Qualität durch strenge Tests
- **Ethik und Gemeinwohl**: Gemeinnützig orientiert, kostenlos verfügbar
- **Dezentralisierung und Unabhängigkeit**: Daten bleiben vor Ort unter voller Kontrolle

## Features von OpenSylab 0.6.0

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

- **User Management System** (NEU in v0.6):
  - Admin-only CRUD interface für Benutzerverwaltung
  - Role-Based Access Control (ADMIN, OPERATOR, VIEWER, CUSTOM)
  - User Profile Management mit Passwortänderung
  - Benutzer aktivieren/deaktivieren
  - Vollständige Benutzerdaten (Name, E-Mail, Rolle)
  - 8 neue API-Endpoints für User Management

- **Audit Log & Compliance** (NEU in v0.6):
  - Kompletter Audit Trail für alle Operationen
  - Admin-only Audit Log Viewer
  - Filterung nach User, Action, Entity, Zeitraum
  - Compliance-Tracking für ISO 15189
  - Unveränderliche Audit-Einträge
  - Detaillierte Aktionsprotokolle

- **Enhanced Dashboard** (NEU in v0.6):
  - Multi-Entity Statistiken (Samples, Orders, Results)
  - Status-Breakdowns mit Zählungen
  - Real-time Daten vom Backend
  - Server-seitige Aggregation
  - Übersichtliche Visualisierung

- **React Web-Interface** (v0.5/v0.6):
  - Moderne Single Page Application (SPA)
  - Responsive Design mit TailwindCSS
  - Dashboard mit umfassenden Statistiken
  - **Probenverwaltung** (Samples): Liste, Filtering, Create/Edit Modals
  - **Auftragsverwaltung** (Orders): Liste, Dual-Filtering (Status + Priority), Create/Edit
  - **Ergebnisverwaltung** (Results): Liste, Dual-Filtering (Status + Flag), Anzeige mit Badges
  - **Benutzerverwaltung** (Users): Admin-only CRUD Interface
  - **Audit Log**: Admin-only Viewer mit Filterung
  - **Profile**: Benutzerprofil mit Passwortänderung
  - Protected Routes mit JWT-Auth und Role-basierter Zugriffskontrolle
  - Real-time Formularvalidierung

- **REST-API** (v0.5/v0.6):
  - Vollständige CRUD-Operationen (GET, POST, PUT, DELETE)
  - **Core Endpoints**: /api/v1/samples, /api/v1/orders, /api/v1/results
  - **User Management** (v0.6): /api/v1/users, /api/v1/users/me, /api/v1/users/me/password
  - **Audit & Stats** (v0.6): /api/v1/audit, /api/v1/stats
  - JSON-basierte Request/Response
  - CORS-Unterstützung für Frontend
  - Konsistente Fehlerbehandlung
  - Audit-Logging für alle Operationen
  - Role-Based Access Control auf API-Ebene

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

### In Entwicklung (v0.7 und später)

- **Sicherheits-Verbesserungen** (v0.7 Priorität):
  - bcrypt/argon2 Password Hashing (aktuell: DJB2, nur für Development!)
  - Secrets Management (JWT-Secret externalisieren)
  - HTTPS-Enforcement für Produktion
- **Frontend CRUD Completion** (v0.7):
  - Create/Edit Forms für Samples, Orders, Results
  - Delete Dialogs mit Soft-Delete
  - Barcode-Scanning Integration
- **Docker-Containerisierung** (v0.7):
  - Multi-stage Dockerfiles
  - Docker Compose für Production
  - Health Checks
- **API-Dokumentation** (v0.7):
  - OpenAPI 3.0 Specification
  - Swagger UI at /api/docs
- **Zukünftige Features**:
  - PostgreSQL-Unterstützung für größere Installationen
  - Qualitätskontrolle (QC)
  - Befunderstellung mit PDF-Export
  - Multi-Tenant-Fähigkeit

Details siehe [ROADMAP.MD](ROADMAP.MD) und [TODO.md](TODO.md).

## Technologie-Stack (v0.6.0)

### Backend

- **Programmiersprache**: C++17
- **Build-System**: CMake 3.15+
- **Datenbank**: SQLite 3.x
- **REST-API**: Custom HTTP-Server mit JSON-Support
- **Authentifizierung**: JWT (jwt-cpp v0.7.0) mit Role-Based Access Control
- **Verschlüsselung**: OpenSSL 3.x für TLS/HTTPS
- **Architektur**: Modulares Design mit klarer Trennung:
  - `core/` - Geschäftslogik und Datenmodelle (Sample, Order, TestResult, User, AuditEntry)
  - `db/` - Datenbankzugriffsschicht mit vollständigem CRUD
  - `api/` - REST-API und HTTP-Server (30+ Endpoints)
  - `auth/` - JWT-Authentifizierung mit RBAC
  - `utils/` - CSV-Import und CLI-Interface

### Frontend

- **Framework**: React 18 mit TypeScript
- **Build-Tool**: Vite 5 mit Hot Module Replacement
- **Styling**: TailwindCSS 3 für responsive UI
- **Routing**: React Router 6 für Single Page Application
- **HTTP-Client**: Axios mit JWT-Interceptors
- **State Management**: React Context API für Auth
- **UI-Komponenten**: Custom Modal-basierte CRUD-Forms
- **Typ-Sicherheit**: Vollständige TypeScript-Typen für alle Entities

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
- **v0.5.0**: REST-API, React Web-UI, JWT-Auth, TLS/HTTPS ✓
- **v0.6.0** (Aktuell): User Management, Audit Log, Enhanced Dashboard, RBAC ✓
- **v0.7.0** (Nächste): Production Security, Docker, OpenAPI, Frontend CRUD
- **v1.0**: Produktionsreifes LIMS mit vollständigem Feature-Set
- **v2.0+**: Multi-Site, KI-Integration

Details siehe [ROADMAP.MD](ROADMAP.MD) und [TODO.md](TODO.md).

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

**Aktuelle Version**: v0.6.0 (2026-02-03)

- ✓ **User Management System** - Admin-only CRUD Interface für Benutzerverwaltung
- ✓ **Audit Log Viewer** - Admin-only Compliance-Tracking mit Filterung
- ✓ **User Profile Management** - Benutzerprofil mit Passwortänderung
- ✓ **Enhanced Dashboard** - Multi-Entity Statistiken mit Status-Breakdowns
- ✓ **Role-Based Access Control** - ADMIN, OPERATOR, VIEWER, CUSTOM Rollen
- ✓ **8 neue API-Endpoints** - User Management, Audit Log, Statistics
- ✓ **JWT Authentication** - Token-basierte Authentifizierung mit Rollenerkennung
- ✓ **React TypeScript Frontend** - Vollständige UI mit TailwindCSS
- ✓ **REST-API** - 30+ Endpoints mit CRUD-Operationen
- ✓ **TLS/HTTPS** - Verschlüsselte Verbindungen
- ✓ **62 Unit-Tests** (Backend)

**Sicherheitshinweis v0.6.0**:
- ⚠️ Default-Credentials: `admin/admin` MÜSSEN in Produktion geändert werden
- ⚠️ Password-Hashing (DJB2) ist NICHT produktionsreif - nur für Development
- ⚠️ JWT-Secret ist hardcoded - muss für Produktion externalisiert werden
- ✓ Diese Punkte werden in v0.7.0 adressiert (bcrypt, Secrets Management)

---

*OpenSylab - Open-Source LIMS für Labore weltweit. Kostenlos, transparent, vertrauenswürdig.*
