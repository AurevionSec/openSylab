# OpenSylab — Roadmap & TODO

**Current version:** v0.9.0 (2026-05-25)
**Next version:** v1.0.0
**Branch:** main

---

## ✅ v0.7.0 — Completed (2026-05-11)

Full change list: [CHANGELOG.md](CHANGELOG.md#070---2026-05-11)

Highlights: JWT Auth · PBKDF2 · RBAC · MFA/TOTP · LDAP · Soft-Delete ·
Auto-Flag · Batch CSV Import · HL7 · FHIR · Audit Trail · 181 unit tests ·
60+ bug fixes from intensive bug hunt · Single Source of Truth for version

---

## ✅ v0.8.x — Completed (2026-05-14)

Full change list: [CHANGELOG.md](CHANGELOG.md)

Highlights: Rate Limiting · Forced password change · Enforce HTTPS · Health endpoint · HL7/FHIR API endpoints · Audit log export · Status transition validation in backend · CI/CD pipeline · 43 bug-hunt iterations · Security hardening · 181 unit tests green

### P0 — Security (blocker for production)

- [x] **No default JWT secret in VCS** — `docker-compose.yml` contains
      `change-this-secret-key-in-production` as a plaintext secret.
      Fix: read secret from file/vault, fail on startup if dev secret is used in prod.
- [x] **Forced password change** on first login with `admin/admin`
      (no first-run guard present — prod deployments run with default credentials)
- [x] **Enforce HTTPS** — `--force-https` flag for prod, HTTP→HTTPS redirect
      (`--tls` flag exists in main.cpp, but no enforcement)
- [x] **Rate Limiting** on `/api/v1/auth/login` — currently zero protection against
      credential stuffing (single-threaded, no login counter)
- [x] **Remove or harden API key fallback** — X-API-Key auth bypassed RBAC
      completely (treated as OPERATOR, no role check possible)
- [x] **Implement socket timeouts** — Missing `SO_RCVTIMEO` / `SO_SNDTIMEO`
      in `ApiServer` makes the system vulnerable to Slowloris DoS attacks.

### P1 — Missing core features

- [x] **Health endpoint** `GET /api/v1/health` — missing entirely;
      Docker health check disabled, reverse proxy probes fail
- [x] **HL7/FHIR API endpoints** — `Hl7Exchange` + `FhirExchange` are
      fully implemented (Hl7.cpp, Fhir.cpp) but not wired in ApiServer.cpp.
      Zero HTTP routes for `/api/v1/hl7/*` and `/api/v1/fhir/*`.
- [x] **Audit log export UI** — `Database::exportAuditLogToCsv()` exists in
      the backend, but no HTTP endpoint and no export button on the audit page
- [x] **Status transition validation in backend** — only enforced in the frontend via
      `SAMPLE_TRANSITIONS`/`ORDER_TRANSITIONS`; the backend accepts any status string
      on PUT (ISO 15189 compliance gap)
- [x] **Configuration file** `opensylab.conf` — done in v0.9.0
- [x] **Frontend unit tests** — done in v0.9.0 (Vitest + RTL, 46 tests)
- [x] **OpenAPI / Swagger** — done in v0.9.0 (docs/openapi.yaml, self-serve endpoint)
- [x] **Database migrations** — done in v0.9.0 (versioned, idempotent)

### P2 — UI/UX improvements

- [x] **Create button on Results page** without `canWrite` guard —
      VIEWER sees the button (fails on submit, but confusing)
- [x] **Breadcrumb bug** — `/audit-log` is displayed as "Dashboard"
      (`routeNames` has `/audit` instead of `/audit-log` in Header.tsx)
- [x] **Search: inconsistent prefix logic** — `O` (without dash) → Orders,
      `R-` (with dash) → Results; `RES-001` silently lands in Samples
- [x] **Import page: samples only** — UI only calls `createSample()`;
      result import (CsvResultImport.cpp), HL7, FHIR are not reachable
- [x] **Dashboard statistics fallback** — with > 100 entries, tiles
      and charts are incomplete (client-side aggregation with limit:100)
- [x] **Order ID in results** — `resultToJson()` sends the numeric PK,
      not the human-readable string like `O-2024-001`; table shows numbers instead of IDs
- [x] **`updated_at` on samples** — `sampleToJson()` has no `updated_at` field;
      frontend always shows `registration_date` as "last modified"
- [x] **Live counters in sidebar badges** — `badge: '24'` in Sidebar is
      hardcoded and never rendered (dead code)
- [x] **Password strength indicator** on the profile page
- [x] **TESTING.md stale** — shows "62 tests" instead of current 181

### P3 — Infrastructure / Operations

- [x] **CI/CD pipeline** — no `.github/workflows/`; no automated
      build/test/push on PR or merge
- [x] **Backend health check in docker-compose.yml** — currently commented out;
      frontend container may start before the backend
- [x] **CORS duplication** — `getenv("OPENSYLAB_CORS_ORIGIN")` is read separately in
      `handleClientTls()` and `handleClientPlain()` (2 locations)
- [x] **Multi-threaded server / concurrency** — done in v0.9.0 (thread-per-connection, max 32 concurrent, 503 on overflow)
- [ ] **Secret key rotation** — no documented process for rotating
      the JWT secret without a server restart

---

## v0.8 overall summary by Effort

| Category | Items | Effort (estimated) |
|----------|-------|--------------------|
| P0 Security | 6 | ~2.5 weeks |
| P1 Core features | 8 | ~4 weeks |
| P2 UI/UX | 10 | ~2 weeks |
| P3 Infrastructure | 5 | ~1.5 weeks |
| **Total** | **29** | **~10 weeks** |

**Recommended MVP scope for v0.8.0** (focused on production readiness):
P0 complete + health endpoint + HL7/FHIR endpoints + breadcrumb fix + TESTING.md

---

## Known technical debt (not a release blocker)

- `token_expiry` key is written in `auth.ts` but read as `_expiry` in `api.ts`
  — if the key is missing, silent logout on every request
- Test runner uses its own macro framework instead of Catch2/GoogleTest → no
  standard CI output without custom scripts
- No E2E/integration tests (Playwright/Cypress)
- **JSON parser limitations** — hand-written parser in `ApiServer.cpp`
  does not support arrays or nested objects; complicates API expansion.

---

## Strategic Analysis Findings (2026-05-16)

### Critical bugs (newly identified)

- [x] **Status enum mismatch frontend/backend** — `constants.ts` defines
      `RESULT_TRANSITIONS` with `REVIEWED`/`AMENDED`; `ApiServer.cpp` uses
      `ENTERED`/`REPEATED`. Frontend shows transitions that the backend rejects.
      Silent ISO 15189 compliance bug. Files: `frontend/src/utils/constants.ts`,
      `src/api/ApiServer.cpp` (`kResultTrans`).
- [x] **SQLite WAL mode not enabled** — Without `PRAGMA journal_mode=WAL` every
      write blocks all concurrent reads. During a batch CSV import the server is
      blocked for all other users for the duration of the import.
      Fix: `PRAGMA journal_mode=WAL;` in `Database::initializeSchema()`.
- [x] **HTTP header truncation at >8192 bytes** — `handleClientPlain` /
      `handleClientTls` read exactly 8192 bytes in the first `recv`/`SSL_read`.
      Very long Authorization headers (e.g. large JWTs, many cookies) are silently
      truncated — auth then fails without an explainable error.
      Fix: header accumulation loop until `\r\n\r\n` found (max 64 KB).
- [x] **No security headers in HTTP responses** — No `Strict-Transport-Security`,
      no `X-Content-Type-Options`, no `X-Frame-Options` in API responses.
      Especially relevant for the plain HTTP path and browser clients.
- [x] **No JWT token blacklisting after logout** — Tokens remain valid until expiry
      (60 min). With compromised credentials the attacker is authorized for that
      window. Fix: in-memory blacklist in ApiRouter + `POST /api/v1/auth/logout`.
- [ ] **TOTP Base32 enrollment flow** — Secrets stored as raw strings are incompatible
      with standard authenticator apps (Google Authenticator, Authy) that expect
      Base32-encoded secrets for QR code enrollment. `base32Decode()` utility added
      to `Database.cpp`. Proper fix requires: (1) new MFA enrollment API endpoint
      that generates a Base32 secret and returns a `otpauth://` URI for QR display,
      (2) DB migration to re-encode existing raw secrets as Base32,
      (3) update `computeMfaCode` to decode Base32 before HMAC.

### Architecture debt (medium to long term)

- [ ] **Split `ApiServer.cpp` (God-file, ~3400 lines)** — All ~30 route handlers,
      JSON parser, URL decoder, rate limiter, CORS logic in a single file.
      Refactoring: one handler class per resource (SampleHandler, OrderHandler,
      ResultHandler, UserHandler, AuditHandler, StatsHandler, HL7Handler, FhirHandler).
      Effort: ~3–4 weeks.
- [x] **Replace JSON parser: nlohmann/json via FetchContent** — done in v0.9.0
- [x] **Layer violation: API imports DB directly** — done in v0.9.0 (`IDatabase` interface; `ApiServer.h` now imports `IDatabase.h` only)
- [x] **PostgreSQL backend option** — done in v0.9.0 (stub; full implementation planned v1.1)

### ✅ v0.9.0 — Architecture modernization (Completed 2026-05-25)

- [x] **Thread pool / thread-per-connection** — thread-per-connection with `std::atomic<int>` slot reservation (max 32 concurrent), `memory_order_seq_cst`, 503 on overflow
- [x] **Socket timeouts** `SO_RCVTIMEO` / `SO_SNDTIMEO` (→ done in v0.8.x)
- [x] **Integrate nlohmann/json** — FetchContent v3.11.3; removed hand-rolled JSON parser; all API endpoints use `json::parse` / `json{}.dump()`
- [x] **SQLite WAL mode** (→ done in v0.8.x)
- [x] **Status enum mismatch** frontend/backend (→ done in v0.8.x)
- [x] **Hash-chain audit trail** — HMAC-SHA256 with canonical JSON serialization; `OPENSYLAB_AUDIT_HMAC_KEY` mandatory (min 32 chars, hard-fail on startup); `BEGIN IMMEDIATE` atomicity; `GET /api/v1/audit/verify` endpoint; tamper detection in unit tests
- [x] **OpenAPI / Swagger** — `docs/openapi.yaml` (OpenAPI 3.0, 23 paths, all schemas); served at `GET /api/v1/openapi.yaml` (unauthenticated, public)
- [x] **Database migrations** — `schema_migrations` table; 3 versioned migrations; idempotent via `PRAGMA table_info` pre-check; runs automatically in `initializeSchema()`
- [x] **PostgreSQL backend option** — `IDatabase` pure-virtual interface; `PostgreSQLDatabase` stub; layer violation in `ApiServer.h` fixed; PostgreSQL startup-blocked until v1.1 (no HMAC support in stub)
- [x] **Structured JSON logs** — spdlog v1.14.1 via FetchContent; console color sink + rotating file sink; `LOG_*` macros throughout; no more `std::cout` in library code
- [x] **TOTP Base32 enrollment flow** — `generateMfaSecret()` with `RAND_bytes`; `base32Encode()`; `getMfaEnrollmentUri()` returning `otpauth://` URI; `POST /auth/mfa/enroll`, `POST /auth/mfa/verify-enrollment`, `DELETE /auth/mfa` endpoints
- [x] **Configuration file** `opensylab.conf` — INI parser; search order: `--config` flag → `OPENSYLAB_CONFIG` env → `./opensylab.conf` → `/etc/opensylab/opensylab.conf`; all settings configurable
- [x] **Frontend unit tests** — Vitest v2.1.9 + React Testing Library; 46 tests across AuthContext, API client, utils, Login component

---

## Strategic roadmap extensions

### STR-1: IQ/OQ/PQ validation package (ISO 15189)

No code — documentation. Installation Qualification (IQ), Operational Qualification
(OQ), Performance Qualification (PQ) per DIN EN ISO 15189 per OpenSylab version.
With automated test scripts that make the OQ process reproducible.

- Target audience: DAkkS-accredited laboratories (~2,000 in Germany), ÖKAS (Austria),
  SAS (Switzerland)
- Market relevance: Validation documentation costs 5,000–20,000 € from external
  consultants; OpenSylab can offer this for 3,000–10,000 € with in-house expertise
- Moat: Only achievable through direct collaboration with accredited laboratories and
  regulatory domain knowledge; a fork cannot replicate this automatically
- Effort: ~6–10 weeks initially, ~2–3 weeks per major version update

### STR-2: ASTM/LIS02-A2 device connectivity

Native C++ implementation of the ASTM E1394/LIS02-A2 protocol (RS-232/TCP-based
standard for instrument-LIS communication). Results arrive directly from the analyzer —
no manual entry. First target instruments: Sysmex XN series (hematology) or
Roche cobas c (clinical chemistry).

- Target audience: Laboratories with automated analyzers — the customers with real budget
- Moat: Instrument-vendor-specific edge cases can only be learned through physical device
  access; whoever has documented them is months ahead
- Effort: ~4–6 weeks per instrument class (with test access or simulator)

### STR-3: Signed Docker image + SBOM ("OpenSylab Verified")

Cosign/Sigstore-signed Docker images, Software Bill of Materials (SPDX/CycloneDX),
public CVE scan dashboard via Trivy in CI/CD. Verification workflow checks image
integrity at startup.

- Target audience: Hospital laboratories in KRITIS-regulated infrastructures, IT security
  officers with requirements for software provenance attestation
- Effort: ~2–3 weeks for setup, then automated in CI/CD

### STR-4: OpenSylab Academy — certification for laboratory informaticists

Paid online training program (Teachable/Podia):
- Module 1: OpenSylab Administration (~200–300 €)
- Module 2: ISO 15189 Compliance with OpenSylab (~300–500 €)
- Module 3: Device integration & API (~200–300 €)
With a recognized certificate "Certified OpenSylab Administrator".

- Target audience: Laboratory informaticists, IT admins in labs, LIMS consultants
- Strategic side effect: Every graduate = potential consulting customer;
  Academy serves simultaneously as a sales funnel
- Effort: ~4–6 weeks for the first course version

---

## Monetization roadmap (recommendation)

**Phase 1 (v0.9–v1.0): Support & Consulting — actionable immediately**
- Deployment & hardening: 1,500–5,000 € per engagement
- ISO 15189 validation documentation for laboratories: 3,000–10,000 €
- Training sessions: 500–1,500 € per session
- Prerequisite: none — revenue-generating immediately

**Phase 2 (v1.0–v1.2): Compliance-as-a-Service + Open Core**
- Productized IQ/OQ/PQ package: 5,000–15,000 € initial validation + 1,500–3,000 €/year
- Open Core: proprietary enterprise features (device connectors STR-2,
  hash-chain audit with eIDAS timestamp, multi-site management)
- MIT core remains public; proprietary features structurally isolated
- Prerequisite: STR-1 (validation package) must be complete

**Phase 3 (v1.3+): Managed SaaS — only after DSGVO clarification**
- Pricing model: Starter 99 €/mo · Professional 299 €/mo · Enterprise 999 €/mo
- Prerequisite: AVV with every customer, BSI C5-certified data center, ISO 27001,
  MDR clarification (medical device?), multi-tenant architecture (roadmap v1.1)
- Timeline: 12–18 months lead time at minimum before first revenue

**Phase 4 (long term): OEM deals, marketplace**
- OEM with mid-sized IVD manufacturers (Sysmex, Mindray) via reference implementations
- Plugin marketplace only makes sense from ~1,000 GitHub stars + active community
- Plugin architecture to be technically prepared now (extension points for device connectors)

---

*Last updated: 2026-05-16 — Strategic Analysis*
