# OpenSylab LIMS

**Open Source Laboratory Information Management System**

[![Version](https://img.shields.io/badge/version-0.7.0-blue)](CHANGELOG.md)
[![License](https://img.shields.io/badge/license-MIT-green)](#lizenz)
[![Tests](https://img.shields.io/badge/tests-181%20passing-brightgreen)](#tests)
[![C++](https://img.shields.io/badge/C%2B%2B-17-orange)](src/)
[![React](https://img.shields.io/badge/React-18-61dafb)](frontend/)

OpenSylab ist ein modernes, quelloffenes LIMS für medizinische Diagnostiklabore.  
Es verwaltet Proben, Aufträge und Testergebnisse mit vollständigem Audit-Trail nach ISO 15189.

---

## Screenshots

> **Hinweis:** Ersetze die Platzhalter durch echte Screenshots aus `http://192.168.10.140:5173`.  
> Empfohlene Ablage: `docs/screenshots/`

### Dashboard
![Dashboard](docs/screenshots/dashboard.png)
*Echtzeit-Übersicht: Proben, Aufträge, kritische Ergebnisse, Aktivitätsdiagramm*

### Proben-Verwaltung
![Samples](docs/screenshots/samples.png)
*Probenliste mit Status-Filter, Suche und Soft-Delete*

### Auftrags-Verwaltung
![Orders](docs/screenshots/orders.png)
*Aufträge nach Priorität und Status (NORMAL / URGENT / EMERGENCY)*

### Ergebnisse & Auto-Flag
![Results](docs/screenshots/results.png)
*Testergebnisse mit automatischer Flaggung: NORMAL / LOW / HIGH / CRITICAL*

### Login & MFA
![Login](docs/screenshots/login.png)
*JWT-Login mit optionalem TOTP-zweitem-Faktor*

### Audit-Log
![Audit](docs/screenshots/audit.png)
*Vollständiger ISO-15189-konformer Audit-Trail mit Filterung und CSV-Export*

### Benutzerverwaltung
![Users](docs/screenshots/users.png)
*RBAC-Rollenverwaltung: ADMIN / OPERATOR / VIEWER / CUSTOM*

---

## Features

### Kernfunktionen
| Feature | Beschreibung |
|---------|-------------|
| **Probenverwaltung** | CRUD, Barcode-Scan, Status-Workflow (REGISTERED → ARCHIVED) |
| **Auftragsverwaltung** | Verknüpfung mit Proben, Prioritäten (NORMAL/URGENT/EMERGENCY) |
| **Ergebniseingabe** | Auto-Flag (NORMAL/LOW/HIGH/CRITICAL) mit Referenzbereich |
| **Batch-Import** | CSV-Import für Proben und Ergebnisse mit Fehlerreporting |
| **HL7 v2.5.1** | ORU^R01 Import/Export mit korrektem Feld-Escaping |
| **FHIR R4** | Bundle Import/Export |
| **Audit-Trail** | Vollständig nach ISO 15189, CSV-Export mit Retention-Policy |
| **Statistiken** | Dashboard-Kacheln, Statusverteilung, kritische Ergebnisse |

### Sicherheit
| Feature | Beschreibung |
|---------|-------------|
| **JWT-Authentifizierung** | HMAC-SHA256, konfigurierbare Ablaufzeit |
| **PBKDF2-Passwort-Hashing** | 210.000 Iterationen (OWASP 2023) |
| **RBAC** | 4 Rollen: ADMIN / OPERATOR / VIEWER / CUSTOM |
| **MFA (TOTP)** | RFC 6238, ±1 Zeitfenster, Google Authenticator kompatibel |
| **LDAP** | Optionale LDAP-Authentifizierung mit lokalem Shadow-Account |
| **Letzter-Admin-Schutz** | Transaktional gesichert gegen Race Conditions |
| **Audit bei allen Writes** | CREATE / UPDATE / DELETE immer mit AuditEntry |

---

## Technologie-Stack

### Backend
- **Sprache**: C++17
- **Datenbank**: SQLite3 (embedded, kein Server nötig)
- **HTTP-Server**: cpp-httplib
- **Auth**: jwt-cpp (HMAC-SHA256)
- **Kryptographie**: OpenSSL (PBKDF2, HMAC-SHA1 für TOTP)
- **Build**: CMake 3.15+

### Frontend
- **Framework**: React 18 + TypeScript (strict mode)
- **Build**: Vite 7
- **Routing**: React Router v6
- **HTTP**: Axios mit JWT-Interceptor
- **Styling**: Tailwind CSS 3
- **Charts**: Recharts

---

## Schnellstart

### Voraussetzungen
- CMake ≥ 3.15
- GCC/Clang mit C++17-Support
- OpenSSL
- Node.js ≥ 18
- SQLite3

### Build & Start

```bash
# 1. Repository klonen
git clone https://github.com/AurevionSec/openSylab.git
cd openSylab

# 2. Backend bauen
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)

# 3. Server starten (Port 9080, DB wird automatisch angelegt)
./build/bin/OpenSylab --api --api-port 9080 --db opensylab.db

# 4. Frontend bauen
cd frontend
npm install
echo "VITE_API_URL=http://localhost:9080/api/v1" > .env.production
npm run build
npx serve dist --listen 5173 --single
```

Dann im Browser: **http://localhost:5173**  
Standard-Login: `admin` / `admin` *(sofort ändern!)*

### Docker

```bash
docker compose up -d
```

Siehe [docs/DOCKER.md](docs/DOCKER.md) für Details.

---

## Konfiguration

| Umgebungsvariable | Standard | Beschreibung |
|-------------------|---------|-------------|
| `OPENSYLAB_JWT_SECRET` | dev-secret | JWT-Signaturschlüssel (in Prod setzen!) |
| `OPENSYLAB_CORS_ORIGIN` | `http://localhost:5173` | Erlaubte Frontend-Origin |
| `OPENSYLAB_DB_PATH` | `opensylab.db` | Datenbankpfad |

```bash
# Produktionsstart
OPENSYLAB_JWT_SECRET="$(openssl rand -hex 32)" \
OPENSYLAB_CORS_ORIGIN="https://lims.meinlabor.de" \
./build/bin/OpenSylab --api --api-port 9080 --db /var/lib/opensylab/lims.db
```

---

## Architektur

```
┌─────────────────────────────────────────────────┐
│  Frontend (React 18 / TypeScript)               │
│  http://localhost:5173                          │
└──────────────────┬──────────────────────────────┘
                   │ HTTPS / JWT
┌──────────────────▼──────────────────────────────┐
│  REST API  (C++17 / cpp-httplib)                │
│  Port 9080  ·  /api/v1/*                        │
│                                                 │
│  Layer 4: ApiServer   (HTTP-Routing, CORS)      │
│  Layer 3: JwtAuth     (Token-Validierung, RBAC) │
│  Layer 2: Utils       (CSV, HL7, FHIR, CLI)     │
│  Layer 1: Database    (SQLite3-Persistenz)      │
│  Layer 0: Core        (Domain-Entities)         │
└──────────────────┬──────────────────────────────┘
                   │
┌──────────────────▼──────────────────────────────┐
│  SQLite3  (embedded)  ·  opensylab.db           │
└─────────────────────────────────────────────────┘
```

---

## API-Übersicht

| Methode | Endpunkt | Beschreibung | Auth |
|---------|----------|-------------|------|
| `POST` | `/api/v1/auth/login` | JWT-Login (+ MFA) | — |
| `GET` | `/api/v1/samples` | Probenliste (`?q=`, `?status=`) | JWT |
| `POST` | `/api/v1/samples` | Probe anlegen | OPERATOR+ |
| `GET` | `/api/v1/orders` | Auftragsliste | JWT |
| `POST` | `/api/v1/orders` | Auftrag anlegen | OPERATOR+ |
| `GET` | `/api/v1/results` | Ergebnisliste (`?flag=`, `?status=`) | JWT |
| `POST` | `/api/v1/results` | Ergebnis eingeben | OPERATOR+ |
| `GET` | `/api/v1/audit` | Audit-Log | ADMIN |
| `GET` | `/api/v1/users` | Benutzerliste | ADMIN |
| `GET` | `/api/v1/stats` | Dashboard-Statistiken | JWT |

Vollständige API-Dokumentation: [docs/API.md](docs/API.md) *(TODO)*

---

## Tests

```bash
# Alle Tests ausführen
cmake --build build && ./build/bin/opensylab_tests
```

**181 Unit-Tests** — Backend (C++): Datenbank, Domain-Entities, API-Router, CSV-Import, HL7, FHIR.

Siehe [docs/TESTING.md](docs/TESTING.md) für Details.

---

## Versionierung

Die Versionsnummer hat **eine** kanonische Quelle:
- **C++**: `CMakeLists.txt` → `project(VERSION x.y.z)` → generiert `include/version.h`
- **Frontend**: `frontend/package.json` → `"version"` → `import.meta.env.VITE_APP_VERSION`

Siehe [docs/VERSIONING.md](docs/VERSIONING.md) für den Release-Prozess.

---

## Mitwirken

1. Fork & Branch anlegen: `git checkout -b feat/mein-feature`
2. Änderungen implementieren + Tests schreiben
3. `cmake --build build && ./build/bin/opensylab_tests` — alle Tests grün
4. `cd frontend && npx tsc -p tsconfig.app.json --noEmit` — kein TypeScript-Fehler
5. Pull Request öffnen

---

## Lizenz

MIT — siehe [LICENSE](LICENSE) *(TODO: anlegen)*

---

## Changelog

Siehe [CHANGELOG.md](CHANGELOG.md) für die vollständige Versionshistorie.

**Aktuelle Version: [0.7.0](CHANGELOG.md#070---2026-05-11)** — JWT-Auth, RBAC, MFA, Auto-Flag, 181 Tests, 60+ Bugfixes.
