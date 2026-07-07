# OpenSylab LIMS

**Open-source LIMS for medical diagnostics — built around an ISO 15189-oriented audit trail, self-hosted, MIT-licensed.**

[![CI](https://github.com/AurevionSec/openSylab/actions/workflows/ci.yml/badge.svg)](https://github.com/AurevionSec/openSylab/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/AurevionSec/openSylab)](https://github.com/AurevionSec/openSylab/releases)
[![License: MIT](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-orange)](src/)
[![React 19](https://img.shields.io/badge/React-19-61dafb)](frontend/)
[![TypeScript strict](https://img.shields.io/badge/TypeScript-strict-blue)](frontend/src/)
[![Security](https://img.shields.io/badge/security-PBKDF2%20%C2%B7%20JWT%20%C2%B7%20TOTP%20%C2%B7%20HMAC--chain-red)](#security)

OpenSylab is a Laboratory Information Management System for small-to-medium diagnostic labs that want a complete, ISO 15189-oriented workflow — Sample → Order → Result → tamper-evident Audit — without enterprise license costs, US-cloud dependencies, or a database server to operate.

![Dashboard](docs/screenshots/dashboard.png)

<details>
<summary><strong>Table of contents</strong></summary>

- [Why OpenSylab?](#why-opensylab)
- [Screenshots](#screenshots)
- [Who is it for?](#who-is-it-for)
- [Scope & limitations](#scope--limitations)
- [Features](#features)
- [Technology stack](#technology-stack)
- [Quick start](#quick-start)
- [Configuration](#configuration)
- [Architecture](#architecture)
- [API overview](#api-overview)
- [Tests](#tests)
- [Contributing](#contributing) · [Security](#security) · [License](#license)

</details>

---

## Why OpenSylab?

### ⚖️ A tamper-evident audit trail at the database layer

Every CREATE / UPDATE / DELETE on samples, orders, results, and users writes an `AuditEntry` (`user_id`, `action`, `entity_type`, `entity_id`, `timestamp`) **at the database layer** — not as an optional application-level feature. Each entry is chained with HMAC-SHA256 over its content plus the previous entry's hash, so the trail cannot be silently edited or bypassed; chain integrity is verifiable via `GET /api/v1/audit/verify`. This is the mechanism ISO 15189 traceability expects, enforced by design.

### 🔌 HL7 v2.5.1 + FHIR R4 — native, no middleware server

Native C++ parsers for HL7 v2.5.1 (`ORU^R01`) and FHIR R4 Bundles (Patient, Specimen, ServiceRequest, Observation) — no separate FHIR middleware server, no Java heap, no external service dependency. Import and export directly via the REST API.

### 🔓 MIT-licensed · self-hosted · no database server

MIT means no copyleft and no license-compliance bureaucracy. SQLite as an embedded database means backup is a file copy, it runs on an inexpensive ARM VM, and no DBA staff is required. Patient data never leaves your own infrastructure.

### 🚀 Minimal footprint via a native C++17 core

No JVM warmup, no interpreter overhead, no framework bloat. OpenSylab runs on hardware where a Java-based LIMS would not start — relevant for edge deployments at the analyzer or resource-constrained on-premise environments.

### 🛡️ Security with concrete mechanisms

PBKDF2-HMAC-SHA256 (210,000 iterations, random salt, constant-time compare), HMAC-SHA256 JWT, TOTP/MFA (RFC 6238), RBAC across four roles (ADMIN / OPERATOR / VIEWER / CUSTOM) with the auth check performed before request-body parsing, login rate-limiting, and TLS enforcement via `--force-https`.

### ⚡ Neo-clinical industrial UI

Design thesis: *user competence over user delight*. A tabular monospace data font so digits align in columns, high contrast, and the information density lab technicians prefer — no generic SaaS styling.

---

## Screenshots

| | |
|---|---|
| ![Samples](docs/screenshots/samples.png) | ![Orders](docs/screenshots/orders.png) |
| **Sample list** — status filter, barcode scan, soft-delete (→ ARCHIVED) | **Orders** by priority (NORMAL / URGENT / EMERGENCY) and status workflow |
| ![Results](docs/screenshots/results.png) | ![Result entry](docs/screenshots/result_create.png) |
| **Results** with automatic flagging: NORMAL / LOW / HIGH / CRITICAL | **Result entry** — auto-flag computed live from the reference range |
| ![Audit log](docs/screenshots/audit.png) | ![Dark mode](docs/screenshots/dark_mode.png) |
| **Audit log** — ISO 15189-oriented trail, filtering, CSV export | **Dark theme** — user-selectable terminal-industrial mode |

---

## Who is it for?

| Target audience | Why OpenSylab fits |
|---|---|
| **Small-to-medium diagnostic laboratories** *(primary)* | Audit trail + RBAC without the six-figure annual license costs of commercial LIMS |
| Research laboratories in university hospitals | Complete Sample → Order → Result → Audit workflow, self-hosted |
| Laboratories with data-protection requirements (GDPR) | Self-hosted, no US-cloud dependency, patient data stays in your infrastructure |
| IT teams without DBA staff | SQLite embedded: no database server, no connection pool, backup via file copy |
| Facilities with LIS/KIS integration | HL7 v2.5.1 + FHIR R4 native — no separate middleware server |

---

## Scope & limitations

OpenSylab is honest about what it is and isn't:

- **Not a certified medical device.** OpenSylab has **not** undergone CE/IVDR marking or FDA clearance. It provides audit-trail and RBAC *mechanisms* that support ISO 15189 workflows, but deploying it in an accredited laboratory requires **your own validation, qualification, and SOPs**. Compliance is granted to a lab's processes, not to a tool.
- **Default `admin` / `admin` credentials are for development only** — change them before any real use, and set `OPENSYLAB_JWT_SECRET` / `OPENSYLAB_AUDIT_HMAC_KEY` to strong random values in production.
- **SQLite is single-writer.** OpenSylab is not intended for high-throughput labs with sustained heavy concurrent writes, nor for SaaS operation without your own IT infrastructure.
- **Provided "as is", without warranty** — see [LICENSE](LICENSE).

---

## Features

<details open>
<summary><strong>Laboratory data management</strong></summary>

| Feature | Description |
|---------|-------------|
| Sample management | CRUD, barcode scan (BarcodeDetector API), workflow REGISTERED → IN_ANALYSIS → ANALYZED → VALIDATED → ARCHIVED |
| Order management | Linked to samples; priorities (NORMAL / URGENT / EMERGENCY); backend-validated status transitions |
| Result entry | Auto-flag on input — NORMAL / LOW / HIGH / **CRITICAL** (margin-based: 50 % of the reference interval), manual override possible |
| Soft-delete | Samples → ARCHIVED, orders → CANCELLED, results → REJECTED — rows retained for the audit trail |
| Reference ranges | Per-result `reference_low` / `reference_high`; flag recomputed on every edit |
| Pagination & search | Server-side pagination (limit/offset, capped) and URL-persisted filters on all list endpoints |
| Migration framework | Versioned schema migrations with checksums, applied at startup |
| OpenAPI spec | Machine-readable spec served at `GET /api/v1/openapi.yaml` |

</details>

<details>
<summary><strong>Import / export</strong></summary>

| Feature | Description |
|---------|-------------|
| CSV import (samples) | RFC 4180, BOM-tolerant, size-limited, per-row error tracking |
| CSV import (results) | Multiline quoted fields with a correct escape state machine |
| HL7 v2.5.1 | ORU^R01 import + export via the REST API, with full field escaping |
| FHIR R4 | Bundle import (Patient, Specimen, ServiceRequest, Observation) + export |
| Audit-log export | CSV export (ADMIN only) with a configurable retention policy |
| Statistics | Backend-aggregated dashboard tiles, status distribution, critical-result count |

</details>

<details>
<summary><strong>Security</strong></summary>

| Feature | Description |
|---------|-------------|
| JWT authentication | HMAC-SHA256, configurable expiry via `OPENSYLAB_JWT_SECRET` |
| Password hashing | PBKDF2-HMAC-SHA256, 210,000 iterations, random salt, constant-time comparison |
| RBAC | ADMIN / OPERATOR / VIEWER / CUSTOM — enforced on every write endpoint, checked before body parsing |
| Rate-limiting | Login endpoint throttled per IP, query-string-resistant |
| MFA (TOTP) | RFC 6238, ±1 window, Google Authenticator compatible |
| LDAP (optional) | LDAP auth with a local shadow account and role mapping |
| Last-admin protection | Demotion/deletion of the last admin blocked transactionally (no TOCTOU) |
| Audit hash chain | Every entry carries an HMAC over its content + previous hash; verifiable via `/api/v1/audit/verify` |
| Concurrency-safe DB | All database access serialized; audit chain atomic under concurrent requests |
| TLS / HTTPS | `--tls-cert` / `--tls-key`; `--force-https` blocks plaintext operation |

Found a vulnerability? Please follow the responsible-disclosure process in [SECURITY.md](SECURITY.md).

</details>

<details>
<summary><strong>Frontend / UX</strong></summary>

| Feature | Description |
|---------|-------------|
| React 19 + TypeScript strict | No `any` in production code, 0 type errors |
| RBAC in the UI | Write actions gated by role; read-only detail view for VIEWER |
| Feedback & a11y | Toasts on every write, focus-trapped modals, `aria-live` errors, skip-link, keyboard nav |
| URL-persisted state | Filters and pagination live in the query string — deep-linkable and back-button-safe |
| Responsive shell | Sidebar collapses to an off-canvas drawer on small screens |
| Dark theme | User-selectable terminal-industrial mode |
| Test pyramid | 46 Vitest component tests + 8 Playwright end-to-end tests (real backend + frontend) in CI |

</details>

### Quality

| Metric | Value |
|--------|------|
| Backend unit tests | 236 passing |
| Frontend tests | 46 Vitest + 8 Playwright E2E |
| TypeScript | strict, 0 errors |
| `npm audit` | 0 vulnerabilities |
| Security alerts | 0 open (Dependabot · CodeQL · secret scanning) |

---

## Technology stack

**Backend** — C++17 · SQLite3 (embedded) · OpenSSL 3.x (EVP_MAC) · nlohmann/json · spdlog · CMake ≥ 3.15
**Frontend** — React 19 · TypeScript (strict) · Vite · React Router · Axios · Tailwind CSS · Recharts

---

## Quick start

### Docker (fastest — try it in two minutes)

```bash
git clone https://github.com/AurevionSec/openSylab.git
cd openSylab
cp .env.example .env         # set OPENSYLAB_JWT_SECRET + OPENSYLAB_AUDIT_HMAC_KEY (openssl rand -hex 32)
docker compose up -d
```

Then open **http://localhost:9090** and sign in with `admin` / `admin` (change it immediately).
See [docs/DOCKER.md](docs/DOCKER.md) for details.

### Build from source

Prerequisites: CMake ≥ 3.15 · GCC/Clang with C++17 · OpenSSL · Node.js ≥ 18 · SQLite3 dev libraries.

```bash
# 1. Clone the repository
git clone https://github.com/AurevionSec/openSylab.git
cd openSylab

# 2. Build the backend
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel "$(nproc)"

# 3. Start the API server (creates the DB automatically)
./build/bin/OpenSylab --api --api-port 8080 --db opensylab.db

# 4. Build & serve the frontend
cd frontend
npm install
echo "VITE_API_URL=http://localhost:8080/api/v1" > .env.production
npm run build
npx serve dist --listen 5173 --single
```

Open **http://localhost:5173** · default login `admin` / `admin` → **change the password immediately.**

---

## Configuration

| Environment variable | Default | Description |
|-------------------|---------|-------------|
| `OPENSYLAB_JWT_SECRET` | — | JWT signing key — **required in production, ≥ 32 chars** |
| `OPENSYLAB_AUDIT_HMAC_KEY` | — | HMAC key for the audit hash chain — **required in production, ≥ 32 chars** |
| `OPENSYLAB_CORS_ORIGIN` | `http://localhost:5173` | Allowed frontend origin |
| `OPENSYLAB_DB_PATH` | `opensylab.db` | Database path |
| `OPENSYLAB_TLS_CERT` / `OPENSYLAB_TLS_KEY` | — | TLS certificate / key (PEM) |

Generate secrets with `openssl rand -hex 32`. Production start with TLS:

```bash
OPENSYLAB_JWT_SECRET="$(openssl rand -hex 32)" \
OPENSYLAB_AUDIT_HMAC_KEY="$(openssl rand -hex 32)" \
OPENSYLAB_CORS_ORIGIN="https://lims.example.org" \
./build/bin/OpenSylab --api --api-port 9443 \
  --tls-cert /etc/ssl/lims.crt --tls-key /etc/ssl/lims.key \
  --force-https --db /var/lib/opensylab/lims.db
```

Secret rotation: [docs/SECRET_ROTATION.md](docs/SECRET_ROTATION.md).

---

## Architecture

```
┌─────────────────────────────────────────────────┐
│  Frontend (React 19 / TypeScript)               │
└──────────────────┬──────────────────────────────┘
                   │ HTTPS / JWT
┌──────────────────▼──────────────────────────────┐
│  REST API (C++17)  ·  /api/v1/*                 │
│                                                 │
│  Layer 4: ApiServer   HTTP routing, TLS, CORS   │
│  Layer 3: JwtAuth     token validation, RBAC    │
│  Layer 2: Utils       CSV, HL7, FHIR, CLI       │
│  Layer 1: Database    SQLite3 persistence       │
│  Layer 0: Core        domain entities           │
└──────────────────┬──────────────────────────────┘
                   │
┌──────────────────▼──────────────────────────────┐
│  SQLite3 (embedded)  ·  opensylab.db            │
└─────────────────────────────────────────────────┘
```

**Layer rule:** layer N may import only layer N-1 — no circular dependencies.

---

## API overview

| Method | Endpoint | Description | Auth |
|---------|----------|-------------|------|
| `GET` | `/api/v1/health` | Health check | — |
| `POST` | `/api/v1/auth/login` | JWT login (+ MFA), rate-limited | — |
| `GET` / `POST` | `/api/v1/samples` | List / create samples | JWT / OPERATOR+ |
| `PUT` | `/api/v1/samples/:id` | Update sample (transition-validated) | OPERATOR+ |
| `GET` / `POST` | `/api/v1/orders` | List / create orders | JWT / OPERATOR+ |
| `GET` / `POST` | `/api/v1/results` | List / enter results (`?flag=`, `?status=`) | JWT / OPERATOR+ |
| `GET` | `/api/v1/audit` | Audit log (filtered, exportable) | ADMIN |
| `GET` | `/api/v1/audit/verify` | Verify audit hash-chain integrity | ADMIN |
| `POST` | `/api/v1/hl7/import` · `/api/v1/fhir/import` | HL7 / FHIR import | OPERATOR+ |
| `GET` | `/api/v1/stats` | Dashboard statistics | JWT |
| `GET` | `/api/v1/openapi.yaml` | OpenAPI 3.0 specification | — |

The full machine-readable contract is in [`docs/openapi.yaml`](docs/openapi.yaml) — load it in Swagger UI or Redoc.

---

## Tests

```bash
# Backend unit tests
cmake --build build && ctest --test-dir build --output-on-failure

# Frontend: type-check, unit tests, end-to-end
cd frontend && npx tsc --noEmit && npm test && npm run test:e2e
```

236 backend unit tests · 46 Vitest component tests · 8 Playwright E2E tests — all run in CI. Details: [docs/TESTING.md](docs/TESTING.md).

---

## Contributing

Contributions are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md) for the workflow and the [Code of Conduct](CODE_OF_CONDUCT.md). In short: fork, branch, add tests, ensure `ctest` and `npx tsc --noEmit` are green, then open a PR (CI must pass).

## Security

Please report vulnerabilities responsibly — see [SECURITY.md](SECURITY.md). Do not open public issues for security problems.

## License

MIT — see [LICENSE](LICENSE).

---

**Roadmap:** [ROADMAP.md](ROADMAP.md) · **Changelog:** [CHANGELOG.md](CHANGELOG.md) · **Current version:** [1.0.0](CHANGELOG.md)
