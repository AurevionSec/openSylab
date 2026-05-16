# OpenSylab — Roadmap & TODO

**Current version:** v0.8.2 (2026-05-14)
**Next version:** v0.9.0
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
- [ ] **Configuration file** `opensylab.conf` — currently everything via CLI flags /
      env vars; no standardized config path for prod deployments
- [ ] **Frontend unit tests** — 0 automated frontend tests (Vitest + RTL).
      Docs: "planned for v0.8.0"
- [ ] **OpenAPI / Swagger** — no machine-readable API contract, 30+ endpoints
      without documentation
- [ ] **Database migrations** — no versioned migration system;
      schema upgrade from v0.7 → v0.8 = manual or DB reset

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
- [ ] **Multi-threaded server / concurrency** — current `serveLoop()` is
      sequential/blocking; a slow or malicious client blocks
      the entire API for all other users. Migration to thread pool
      or thread-per-connection required.
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
- [ ] **HTTP header truncation at >8192 bytes** — `handleClientPlain` /
      `handleClientTls` read exactly 8192 bytes in the first `recv`/`SSL_read`.
      Very long Authorization headers (e.g. large JWTs, many cookies) are silently
      truncated — auth then fails without an explainable error.
      Fix: header accumulation analogous to body accumulation via Content-Length.
- [x] **No security headers in HTTP responses** — No `Strict-Transport-Security`,
      no `X-Content-Type-Options`, no `X-Frame-Options` in API responses.
      Especially relevant for the plain HTTP path and browser clients.
- [ ] **No JWT token blacklisting after logout** — Tokens remain valid until expiry
      (60 min). With compromised credentials the attacker is authorized for that
      window. Fix: Redis-based blacklist or short-lived tokens + refresh token rotation.
- [ ] **No TOTP Base32 enrollment flow documented** — TOTP secrets appear to be stored
      as raw strings. Verify compatibility with standard authenticator apps
      (Google Authenticator, Authy) via QR code enrollment.

### Architecture debt (medium to long term)

- [ ] **Split `ApiServer.cpp` (God-file, ~3400 lines)** — All ~30 route handlers,
      JSON parser, URL decoder, rate limiter, CORS logic in a single file.
      Refactoring: one handler class per resource (SampleHandler, OrderHandler,
      ResultHandler, UserHandler, AuditHandler, StatsHandler, HL7Handler, FhirHandler).
      Effort: ~3–4 weeks.
- [ ] **Replace JSON parser: nlohmann/json via FetchContent** — The hand-rolled
      implementation does not support arrays or nested objects. nlohmann/json is
      header-only, zero transitive dependencies, excellent security track record.
      FetchContent infrastructure already exists (jwt-cpp). Effort: ~1 week.
- [ ] **Layer violation: API imports DB directly** — `include/api/ApiServer.h`
      imports `db/Database.h` directly; violates the documented 5-layer rule
      (Layer 4 must not import Layer 1). Introduce a repository/service pattern
      as a mediator long term.
- [ ] **PostgreSQL backend option** — SQLite is correct for the single-lab edition;
      for multi-site / multi-tenant (roadmap v1.1) a DB abstraction layer with
      PostgreSQL support is needed. Roadmap v0.9 foresees this.

### v0.9.0 — Architecture modernization (priority proposal)

- [ ] **Thread pool / thread-per-connection** (→ existing in P3, prioritized here)
- [ ] **Socket timeouts** `SO_RCVTIMEO` / `SO_SNDTIMEO` (→ existing in P0)
- [ ] **Integrate nlohmann/json** (→ new, see above)
- [ ] **SQLite WAL mode** (→ new, see above)
- [ ] **Status enum mismatch** frontend/backend (→ new, see above)
- [ ] **Hash-chain audit trail** — Cryptographically chained audit entries: each
      hash includes the hash of the previous entry. A tampered entry breaks the
      entire chain — mathematically provable. Optional: RFC 3161-qualified
      timestamp (QTSP per eIDAS) per hash for forensic admissibility.
      Effort: ~3–4 weeks (implementation + verification tool + documentation).
- [ ] **OpenAPI / Swagger** (→ existing in P1)
- [ ] **Database migrations** (→ existing in P1)
- [ ] **PostgreSQL backend option** (→ new, see above)
- [ ] **Structured JSON logs** — No dedicated logging system currently;
      `std::cout`/`std::cerr` in library code violates the CLAUDE.md rule.

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
