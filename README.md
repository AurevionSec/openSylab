# OpenSylab LIMS

**Open-source LIMS for medical diagnostics — ISO 15189-compliant, self-hosted, MIT-licensed.**

[![Version](https://img.shields.io/badge/version-0.9.0-blue)](CHANGELOG.md)
[![License](https://img.shields.io/badge/license-MIT-green)](#license)
[![Tests](https://img.shields.io/badge/tests-228%20passing-brightgreen)](#tests)
[![C++](https://img.shields.io/badge/C%2B%2B-17-orange)](src/)
[![React](https://img.shields.io/badge/React-18-61dafb)](frontend/)
[![TypeScript](https://img.shields.io/badge/TypeScript-strict-blue)](frontend/src/)
[![Security](https://img.shields.io/badge/security-PBKDF2%20%7C%20JWT%20%7C%20TOTP%20%7C%20HMAC--chain-red)](#security)

OpenSylab is a LIMS for small to medium diagnostic laboratories that want to run a complete, ISO 15189-capable system — without enterprise license costs, without US-cloud dependencies, without a database server.

---

## Why OpenSylab?

### ⚖️ ISO 15189 Audit Trail as an Architectural Foundation

Every CREATE / UPDATE / DELETE operation on samples, orders, results, and users mandatorily creates an `AuditEntry` with `user_id`, `action`, `entity_type`, `entity_id`, and `timestamp` — directly at the database layer, not as an optional feature. No other open-source LIMS solution implements this as an inalienable foundation. Compliance here is not a module you bolt on.

### 🔌 HL7 v2.5.1 + FHIR R4 — native, no middleware server

Native C++ parsers for HL7 v2.5.1 (`ORU^R01`) and FHIR R4 Bundles (Patient, Specimen, ServiceRequest, Observation) — no separate FHIR middleware server, no Java heap, no dependency on external services. Import and export directly via the REST API.

### 🔓 MIT License · Self-Hosted · No Database Server Dependency

MIT license means: no copyleft, no license-compliance bureaucracy for IT departments, no restrictions on commercial development. SQLite as an embedded database: backup = copy a file, runs on an inexpensive ARM VM, no DBA staff required. Patient data never leaves your own infrastructure.

### 🚀 Minimal Resource Footprint via Native C++17 Core

No JVM warmup, no interpreter overhead, no framework bloat. OpenSylab runs on hardware where no Java-based LIMS would even start — relevant for edge deployments directly at the analyzer or resource-constrained on-premise environments.

### 🛡️ Security with Concrete Mechanisms

PBKDF2-HMAC-SHA256 (210,000 iterations, random salt), HMAC-SHA256 JWT, TOTP/MFA (RFC 6238), RBAC across four roles (ADMIN / OPERATOR / VIEWER / CUSTOM) with auth check before JSON parse, rate-limiting on the login endpoint, TLS enforcement via `--force-https`.

### ⚡ Neo-Clinical Industrial UI

Design philosophy: "User Competence over User Delight." Monospace data font for tabular digits, high contrast, information density that lab technicians prefer. No generic SaaS styling.

---

## Screenshots

### Dashboard
<!-- SCREENSHOT: Hauptansicht nach Login -->
<!-- Aufnahme: http://192.168.10.140:5173/ (als admin eingeloggt) -->
<!-- Zeigt: KPI-Kacheln (Proben, Aufträge, Ergebnisse, Kritisch), Aktivitäts-Charts, letzte Proben-Tabelle -->
![Dashboard](docs/screenshots/dashboard.png)
*Real-time overview: samples, orders, critical results, and activity chart*

### Sample Management
<!-- SCREENSHOT: Seite /samples mit mindestens 5 Einträgen verschiedener Status -->
<!-- Aufnahme: Filter auf "Alle Status", Suche leer, erste Seite -->
<!-- Zeigt: Tabelle mit sample_id, patient_id, Status-Badge, Aktions-Buttons -->
![Samples](docs/screenshots/samples.png)
*Sample list with status filter, barcode scan, and soft-delete (status → ARCHIVED)*

### Order Management
<!-- SCREENSHOT: Seite /orders mit Aufträgen verschiedener Prioritäten -->
<!-- Aufnahme: Enthält mindestens je einen NORMAL, URGENT, EMERGENCY-Auftrag -->
<!-- Zeigt: Prioritäts-Badge-Farben (grau/orange/rot), Status-Workflow-Spalte -->
![Orders](docs/screenshots/orders.png)
*Orders by priority (NORMAL / URGENT / EMERGENCY) and status workflow*

### Results & Auto-Flag
<!-- SCREENSHOT: Seite /results mit Ergebnissen verschiedener Flags -->
<!-- Aufnahme: Enthält NORMAL (grün), LOW (blau), HIGH (orange), CRITICAL (rot) Flags -->
<!-- Zeigt: Flag-Farb-Badges, Referenzbereich-Spalte, numerischer Messwert -->
![Results](docs/screenshots/results.png)
*Test results with automatic flagging: NORMAL / LOW / HIGH / CRITICAL*

### Record Result — Reference Range & Auto-Flag
<!-- SCREENSHOT: Modal "Neues Ergebnis" geöffnet, Wert außerhalb Referenzbereich eingegeben -->
<!-- Aufnahme: Wert = 15.2, Ref Min = 4.0, Ref Max = 11.0 → Flag zeigt automatisch "HIGH" -->
<!-- Zeigt: Formular mit Auto-Flag-Berechnung in Echtzeit -->
![Result Create](docs/screenshots/result_create.png)
*Result entry with automatic flag calculation on deviation from the reference range*

### Audit Log
<!-- SCREENSHOT: Seite /audit-log mit mehreren Einträgen, Filter-Panel oben -->
<!-- Aufnahme: Mindestens CREATE, UPDATE, DELETE Aktionen sichtbar; Export-Button sichtbar -->
<!-- Zeigt: Tabelle mit Timestamp, Benutzer, Aktion, Entity-Typ, Details -->
![Audit](docs/screenshots/audit.png)
*Complete ISO-15189-compliant audit trail with filtering and CSV export*


### Dark Mode 
<!-- SCREENSHOT: Dashboard im Dark Mode (sudo-Tastenkombination aktiviert) -->
<!-- Aufnahme: s-u-d-o eingeben außerhalb Inputs → Terminal-Industrial-Theme -->
<!-- Zeigt: Acid-Green (#CCFF00), Cyan (#00F0FF) Akzente auf dunklem Hintergrund -->
![Dark Mode](docs/screenshots/dark_mode.png)
*Terminal Industrial Dark Mode with neon accents — activated via `sudo` key sequence*

---

## Who is OpenSylab for?

| Target audience | Why OpenSylab fits |
|---|---|
| **Small to medium diagnostic laboratories** | ISO 15189 audit trail + RBAC without enterprise license costs (LabWare, STARLIMS: from USD 100,000/year) |
| **Research laboratories in university hospitals** | Complete workflow (Sample → Order → Result → Audit) at clinical compliance level |
| **Laboratories with data protection requirements (GDPR)** | Self-hosted, no US-cloud dependency, patient data in your own infrastructure |
| **IT teams without DBA staff** | SQLite embedded: no database server, no connection pool, backup via file copy |
| **Facilities with KIS integration** | HL7 v2.5.1 + FHIR R4 native — no separate middleware server required |

OpenSylab is **not** suitable for: high-throughput laboratories with >100 concurrent write operations (SQLite single-writer limit), laboratories that require SaaS operation without their own IT infrastructure.

---

## Features — v0.9.0

### Laboratory Data Management
| Feature | Description |
|---------|-------------|
| **Sample management** | CRUD, barcode scan (BarcodeDetector API), status workflow: REGISTERED → IN_ANALYSIS → ANALYZED → VALIDATED → ARCHIVED |
| **Order management** | Linked to samples, priorities (NORMAL / URGENT / EMERGENCY), status workflow, transition validation in the backend |
| **Result entry** | Auto-flag on input: NORMAL / LOW / HIGH / **CRITICAL** (margin-based: 50 % of the reference interval) — manual override possible |
| **Soft-delete** | Samples → ARCHIVED, orders → CANCELLED, results → REJECTED — rows are retained for the audit trail |
| **Reference ranges** | Per result `reference_low` / `reference_high`, flag recalculated on every update |
| **Pagination** | Server-side pagination on all list endpoints (limit / offset, capped at 1 000) |
| **Global search** | Header search bar navigates via `?q=` to samples or orders |
| **Migration framework** | Versioned DB schema migrations with checksums, run at startup |
| **OpenAPI spec** | Machine-readable spec served at `GET /api/v1/openapi.yaml` |

### Data Import / Export
| Feature | Description |
|---------|-------------|
| **Batch CSV import (samples)** | RFC 4180, BOM-tolerant, 5 MB limit, per-row error tracking |
| **Batch CSV import (results)** | Multiline fields with correct escape state machine |
| **HL7 v2.5.1** | ORU^R01 import + export via HTTP API (`/api/v1/hl7/import`) with complete field escaping |
| **FHIR R4** | Bundle import (Patient, Specimen, ServiceRequest, Observation) + export via HTTP API |
| **Audit log export** | CSV export (ADMIN only) with configurable retention policy |
| **Statistics** | Dashboard tiles, status distribution, critical results (real-time, backend-aggregated) |

### Security
| Feature | Description |
|---------|-------------|
| **JWT authentication** | HMAC-SHA256, expiry configurable via `OPENSYLAB_JWT_SECRET` |
| **PBKDF2 password hashing** | 210,000 iterations, random salt, empty-salt guard, constant-time comparison (OWASP 2023) |
| **RBAC** | 4 roles: ADMIN / OPERATOR / VIEWER / CUSTOM — enforced on all write endpoints, auth check before JSON parse |
| **Rate-limiting** | 10 attempts / 60 s per IP on `/api/v1/auth/login`, query-string-resistant |
| **MFA (TOTP)** | RFC 6238 HMAC-SHA1, ±1 time window (90 s), Google Authenticator compatible |
| **LDAP** | Optional LDAP authentication with local shadow account and role mapping |
| **Last-admin protection** | `updateUser`, `deleteUser`, `assignUserRole` block demotion of the last admin — transactional, no TOCTOU |
| **Self-delete guard** | Admin cannot deactivate themselves |
| **Audit on all writes** | Every CREATE / UPDATE / DELETE produces an AuditEntry with `user_id`, `action`, `entity`, `timestamp` |
| **HMAC-SHA256 audit hash chain** | Every audit entry carries an HMAC over its content + previous hash; chain integrity verifiable via `GET /api/v1/audit/verify` |
| **Error sanitization** | HTTP responses contain no SQLite internals |
| **TLS/HTTPS** | `--tls-cert` / `--tls-key` flags; `--force-https` prevents HTTP operation in production |

### Frontend / UX
| Feature | Description |
|---------|-------------|
| **React 19 + TypeScript strict** | No `any` types in production code, 0 TypeScript errors |
| **RBAC in the UI** | `canWrite` guards on all Create/Edit/Delete buttons; sidebar links role-dependent |
| **Dark mode** | Terminal-industrial aesthetic via `sudo` key sequence (Easter egg) |
| **Responsive tables** | Secondary columns hidden on tablet (`md:hidden`) |
| **useEntityList hook** | Universal paginated list hook with abort mechanism, race-condition-safe |
| **MFA login flow** | Two-step login: credentials → TOTP code |
| **Secure logout navigation** | Sidebar logout navigates immediately to `/login` |
| **Health endpoint** | `GET /api/v1/health` — unauthenticated, returns `{"status":"ok","service":"opensylab-lims"}` |
| **Version SSOT** | `CMakeLists.txt` → `include/version.h` (C++); `package.json` → `VITE_APP_VERSION` (frontend) |
| **IDatabase interface** | Abstraction layer for testability; `Database` (SQLite) and `PostgreSQLDatabase` both implement `IDatabase` |

### Quality
| Metric | Value |
|--------|------|
| **Unit tests** | 228 passing |
| **TypeScript** | strict mode, 0 errors |
| **npm audit** | 0 vulnerabilities |
| **Bug hunts (v0.9.0)** | 36 waves, 3 consecutive APPROVED — 28 bugs fixed across 21 files |
| **Total bug hunts** | 80+ waves total, 120+ bugs fixed |

---

## Technology Stack

### Backend
- **Language**: C++17
- **Database**: SQLite3 (embedded, no external server required)
- **Auth**: PBKDF2-HMAC-SHA256 (passwords) · HMAC-SHA256 JWT · HMAC-SHA1 TOTP
- **Cryptography**: OpenSSL 3.x (EVP_MAC API)
- **JSON**: nlohmann/json
- **Logging**: spdlog
- **Build**: CMake 3.15+

### Frontend
- **Framework**: React 19 + TypeScript (strict mode)
- **Build**: Vite 7
- **Routing**: React Router v6
- **HTTP**: Axios with JWT interceptor + token-expiry guard
- **Styling**: Tailwind CSS 3
- **Charts**: Recharts

---

## Quick Start

### Prerequisites
- CMake ≥ 3.15
- GCC / Clang with C++17 support
- OpenSSL
- Node.js ≥ 18
- SQLite3 (development libraries)

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

# 4. Frontend bauen & starten
cd frontend
npm install
echo "VITE_API_URL=http://localhost:9080/api/v1" > .env.production
npm run build
npx serve dist --listen 5173 --single
```

Open browser: **http://localhost:5173**  
Default login: `admin` / `admin` → **change password immediately!**

### Docker

```bash
docker compose up -d
```

Detailed instructions: [docs/DOCKER.md](docs/DOCKER.md)

---

## Configuration

| Environment variable | Default | Description |
|-------------------|---------|-------------|
| `OPENSYLAB_JWT_SECRET` | dev-secret | JWT signing key **(in production: generate randomly, ≥32 chars!)** |
| `OPENSYLAB_AUDIT_HMAC_KEY` | — | HMAC key for audit hash chain **(mandatory in production, ≥32 chars; generate with `openssl rand -hex 32`)** |
| `OPENSYLAB_CORS_ORIGIN` | `http://localhost:5173` | Allowed frontend origin |
| `OPENSYLAB_DB_PATH` | `opensylab.db` | Database path |
| `OPENSYLAB_TLS_CERT` | — | Path to TLS certificate (PEM) |
| `OPENSYLAB_TLS_KEY` | — | Path to TLS key (PEM) |

```bash
# Produktionsstart mit TLS
OPENSYLAB_JWT_SECRET="$(openssl rand -hex 32)" \
OPENSYLAB_CORS_ORIGIN="https://lims.meinlabor.de" \
./build/bin/OpenSylab \
  --api --api-port 9443 \
  --tls-cert /etc/ssl/lims.crt \
  --tls-key  /etc/ssl/lims.key \
  --force-https \
  --db /var/lib/opensylab/lims.db
```

---

## Architecture

```
┌─────────────────────────────────────────────────┐
│  Frontend (React 19 / TypeScript)               │
│  http://localhost:5173                          │
└──────────────────┬──────────────────────────────┘
                   │ HTTPS / JWT
┌──────────────────▼──────────────────────────────┐
│  REST API  (C++17)                              │
│  Port 9080/9443  ·  /api/v1/*                  │
│                                                 │
│  Layer 4: ApiServer   (HTTP-Routing, TLS, CORS) │
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

**Layer rule:** Layer N may only import Layer N-1 — no circular dependencies.

---

## API Overview

| Method | Endpoint | Description | Auth |
|---------|----------|-------------|------|
| `GET` | `/api/v1/health` | Health check + version | — |
| `POST` | `/api/v1/auth/login` | JWT login (+ MFA) — rate-limited | — |
| `GET` | `/api/v1/samples` | Sample list (`?q=`, `?status=`) | JWT |
| `POST` | `/api/v1/samples` | Create sample | OPERATOR+ |
| `PUT` | `/api/v1/samples/:id` | Update sample (transition validated) | OPERATOR+ |
| `GET` | `/api/v1/orders` | Order list | JWT |
| `POST` | `/api/v1/orders` | Create order | OPERATOR+ |
| `GET` | `/api/v1/results` | Result list (`?flag=`, `?status=`) | JWT |
| `POST` | `/api/v1/results` | Enter result | OPERATOR+ |
| `GET` | `/api/v1/audit` | Audit log (filtered, exportable) | ADMIN |
| `GET` | `/api/v1/users` | User list | ADMIN |
| `POST` | `/api/v1/hl7/import` | HL7 v2.5.1 ORU^R01 import | OPERATOR+ |
| `POST` | `/api/v1/fhir/import` | FHIR R4 Bundle import | OPERATOR+ |
| `GET` | `/api/v1/stats` | Dashboard statistics (backend-aggregated) | JWT |
| `GET` | `/api/v1/audit/verify` | Verify HMAC audit hash chain integrity | ADMIN |
| `GET` | `/api/v1/openapi.yaml` | OpenAPI 3.0 specification | — |

Full API documentation: [docs/API.md](docs/API.md)

---

## Tests

```bash
# Backend-Tests ausführen
cmake --build build && ./build/bin/opensylab_tests

# Frontend-Typprüfung
cd frontend && npx tsc --noEmit

# CI-äquivalent (mit Timeout)
timeout 60 ./build/bin/opensylab_tests
```

**228 unit tests passing** — backend (C++): database, domain entities, API router, CSV import, HL7, FHIR, statistics, utils.

Further details: [docs/TESTING.md](docs/TESTING.md)

---

## Versioning

The version number has **one** canonical source per layer:
- **C++**: `CMakeLists.txt` → `project(VERSION x.y.z)` → generates `include/version.h` in the build tree
- **Frontend**: `frontend/package.json` → `"version"` → `import.meta.env.VITE_APP_VERSION`

See [docs/VERSIONING.md](docs/VERSIONING.md) for the release process.

---

## Contributing

1. Fork & branch: `git checkout -b feat/my-feature`
2. Implement + write tests
3. `cmake --build build && timeout 60 ./build/bin/opensylab_tests` — all tests green
4. `cd frontend && npx tsc --noEmit` — 0 TypeScript errors
5. Open a pull request

---

## License

MIT — see [LICENSE](LICENSE)

---

## Built with

Developed with [Claude Code](https://claude.ai/code) (Anthropic).

---

## Changelog

Full version history: [CHANGELOG.md](CHANGELOG.md)

**Current version: [0.9.0](CHANGELOG.md#090)** — architecture modernization: IDatabase interface, nlohmann/json, spdlog, migration framework, OpenAPI spec, HMAC audit hash chain, 228 tests passing.
