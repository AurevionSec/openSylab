# Changelog

All notable changes to OpenSylab are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres
to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.1.0] - 2026-07-06

Post-1.0 hardening, a full UX overhaul, and an outward-presentation pass. Backed by
multi-agent audits (a 28-finding bug/inconsistency audit, a ~82-finding UX survey,
and a presentation review). 236 backend + 46 frontend + 8 E2E tests, all green in CI.

### Added

- **Toast notifications** on every create / update / delete (writes are no longer silent).
- **Read-only detail view** (`DetailModal`) reachable by every role, including VIEWER.
- **Responsive sidebar drawer** — off-canvas navigation with a hamburger on small screens.
- **URL-persisted filters & pagination** on all list pages (deep-linkable, back-button-safe).
- **Cross-entity navigation** — Sample ↔ Order ↔ Result IDs are now links.
- Shared **`StatusBadge`** (rectangular tags) and a `useModalA11y` focus-trap hook.
- **Playwright end-to-end suite** (8 tests: login, navigation, API read, mobile drawer) wired into CI.
- `CITATION.cff` and a branded social-preview card.

### Changed

- **Activated the design-token layer** — the project runs Tailwind v4, which ignored
  `tailwind.config.js`; a v4 `@theme` block now makes the Neo-Clinical palette and the
  JetBrains Mono data font actually render.
- **Accessibility** — focus-trapped modals with Esc/restore, `aria-live` error/status
  regions, a skip-to-content link, `aria-current` navigation, associated form labels.
- **Flat surfaces** (borders instead of shadows) per the design language.
- **README rewritten** — English-only, ISO 15189 wording tightened to "-oriented", a
  Scope & limitations disclaimer, live CI/release badges, Docker-first quick start.
- **Docs de-staled** — correct Docker ports, current versions, complete env-var tables,
  fixed dead links; new `docs/README.md` index. Fresh 1.0 screenshots.

### Fixed

- **Concurrency (critical):** serialize the shared SQLite connection with a recursive
  mutex + `busy_timeout` — concurrent request threads could otherwise collide on
  transactions and corrupt the audit hash chain.
- **RBAC:** sample → VALIDATED release now requires ADMIN (matching order/result); the
  JWT effective role is derived from the live DB user, so role changes apply immediately.
- **Audit trail:** HL7/FHIR exports now log PHI disclosure; `logAudit` fails closed
  instead of writing a chain-breaking row on HMAC failure.
- **Data integrity:** CSV result import no longer mis-splits quoted fields or coerces
  malformed reference bounds to `0.0`; NaN/inf values are no longer flagged NORMAL; the
  result-edit modal recomputes the flag when the value/range changes.
- **Frontend:** user roles normalized (fixes blank badges + an edit-dropdown privilege
  hazard); LOGIN_FAILED shown in the audit log; global search routes orders/results
  correctly; a stale token no longer blocks login; list-refetch races guarded; fixed the
  Tailwind-v4 modal-backdrop regression.

### Security

- Externalize Docker Compose secrets to an untracked `.env`; fix the Dockerfile
  healthcheck; untrack the `.vite` build cache and debug HTML.

## [1.0.0] - 2026-07-05

First 1.0 release. Consolidates the post-0.9.0 hardening, governance,
architecture, and test work.

### Architecture

- **`ApiServer.cpp` God-function decomposition (Phase A)** (#48, #62, #63) —
  `ApiRouter::handleRequest` reduced from **2781 → 440 lines** by extracting
  every inline route branch into a dedicated per-route handler method, threading
  a shared `RouteContext` (write handlers also receive the parsed body map).
  All ~30 routes extracted (health/openapi, audit, stats, HL7/FHIR, MFA, users,
  and samples/orders/results across GET/POST/PUT/DELETE). Behaviour-preserving:
  each step verified by an independent review plus the full unit-test suite.

### Security

- **Resolved all open Dependabot (11) and CodeQL (3) alerts** (#46): `vitest`
  2.x → 3.2.6 (critical — arbitrary file read/exec via UI server),
  `vite` → 7.3.5 (high — `server.fs.deny` bypass), `react-router-dom` → 7.15.1
  (CSRF), plus transitive fixes (`form-data`, `@babel/core`, `esbuild`,
  `js-yaml`); `npm audit` clean. Least-privilege `permissions: contents: read`
  added to the CI workflow. Test-data generator (`tools/generate_testdata.py`)
  now hashes with PBKDF2-HMAC-SHA256 in the backend format instead of plain
  SHA256.

### Added

- **Project governance** — `SECURITY.md` (private vulnerability reporting),
  `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md` (Contributor Covenant 2.1),
  `.github/ISSUE_TEMPLATE/` and `PULL_REQUEST_TEMPLATE.md`.
- **`docs/SECRET_ROTATION.md`** — JWT-secret rotation runbook, including the
  caveat that `OPENSYLAB_AUDIT_HMAC_KEY` must not be rotated on a populated
  database (breaks the audit hash chain).
- **`TODO.md`** — v1.0.0 release roadmap.

### CI / tooling

- Bump `actions/checkout` and `actions/setup-node` to v5; Node.js 20 → 22.
- Set `OPENSYLAB_JWT_SECRET` for the CI test runner; remove unused test imports.
- Fix `.github/dependabot.yml`: invalid `package-ecosystem: "OpenSylab"` replaced
  with real `npm` (`/frontend`) and `github-actions` (`/`) ecosystems.

### Fixed (documentation)

- Corrected stale framework version in README docs: React 18 → 19 (matches
  `frontend/package.json`).
- Corrected the `/api/v1/health` response example to
  `{"status":"ok","service":"opensylab-lims"}` (the endpoint returns no
  `version` field).

### Tests

- **API-layer compliance coverage** (#64) — HTTP-level regression tests through
  `ApiRouter::handleRequest` for the ISO 15189 branches relocated during the
  refactor: immutability guards (409), invalid status-transition rejection (409),
  ADMIN-only VALIDATE enforcement (403), and delete guards (active-orders 409,
  idempotent REJECTED 204, VALIDATED 409). Suite now at 235 unit tests.

## [0.9.0] - 2026-05-25

### Architecture

- **nlohmann/json v3.11.3** — FetchContent integration; replaced hand-rolled JSON parser throughout `ApiServer.cpp`; all API endpoints now use `json::parse` / `json{}.dump()`
- **spdlog v1.14.1** — Structured JSON logging via FetchContent; console color sink + optional rotating file sink; `LOG_*` macros replace all `std::cout`/`std::cerr` in library code
- **IDatabase interface** — Pure virtual `IDatabase` interface introduced; `ApiServer.h` now imports `IDatabase.h` (Layer 4 → Layer 2 → Layer 1 violation fixed); all components depend only on the interface
- **PostgreSQLDatabase stub** — `PostgreSQLDatabase` implements `IDatabase`; all methods return safe failure values; startup guard in `main.cpp` rejects `dbBackend=postgresql` until v1.1 (no HMAC support)

### Security

- **HMAC-SHA256 audit hash chain** — Each audit entry is HMAC-SHA256-chained to its predecessor using canonical JSON field serialization (collision-safe); `OPENSYLAB_AUDIT_HMAC_KEY` is mandatory at startup (min 32 chars, hard-fail); `BEGIN IMMEDIATE` transactions ensure atomic hash-chain writes; tamper detection verified in unit tests
- **`GET /api/v1/audit/verify`** — New ADMIN-only endpoint verifies the complete hash chain and returns the first broken entry on integrity failure
- **Startup guards** — Hard-fail if `OPENSYLAB_AUDIT_HMAC_KEY` is absent or shorter than 32 characters; hard-fail if `dbBackend=postgresql` (stub has no HMAC support)
- **`docker-compose.yml`** — Added `OPENSYLAB_AUDIT_HMAC_KEY` placeholder with `openssl rand -hex 32` generation instruction

### Features

- **Thread-per-connection concurrency** — `serveLoop()` now spawns a `std::thread` per accepted connection; `std::atomic<int>` slot reservation with `memory_order_seq_cst`; max 32 concurrent connections (HTTP 503 on overflow)
- **Database migrations** — `schema_migrations` table tracks applied migrations; 3 versioned migrations (chain_hash column, mfa_secret_base32 column, auth_config table); idempotent via `PRAGMA table_info` pre-check; runs automatically on `initializeSchema()`
- **TOTP Base32 enrollment flow** — `generateMfaSecret()` generates cryptographic secret via `RAND_bytes`; `base32Encode()` produces authenticator-compatible Base32; `getMfaEnrollmentUri()` returns `otpauth://totp/...` URI for QR display; `POST /auth/mfa/enroll`, `POST /auth/mfa/verify-enrollment`, `DELETE /auth/mfa` endpoints
- **Configuration file** `opensylab.conf` — INI-format config with search order: `--config` flag → `OPENSYLAB_CONFIG` env var → `./opensylab.conf` → `/etc/opensylab/opensylab.conf`; all 11 settings configurable; explicit paths that don't exist cause startup failure; `..` in path rejected
- **OpenAPI 3.0 specification** — `docs/openapi.yaml` (23 paths, all schemas, BearerAuth security scheme); served unauthenticated at `GET /api/v1/openapi.yaml`; compatible with Swagger UI, Redoc, and `openapi-generator`
- **Frontend unit tests** — Vitest v2.1.9 + React Testing Library + happy-dom; 46 tests covering AuthContext (7), API client (6), utility functions (24), Login component (9)

### Bug fixes

- **Unused includes removed** — `<algorithm>` from `Fhir.cpp`, `<chrono>`/`<thread>` from `CliInterface.cpp`, `<sstream>` from `CsvImport.cpp`, `<stdexcept>` from `Logger.cpp`, `<cerrno>` from `ApiServer.cpp`

## [0.8.2] - 2026-05-14

### Bug fixes (Tests & Validation)
- **include/core/Sample.h / include/core/Order.h**: Add new validation methods (`isValidStatusString`, `isValidPriorityString`) for Sample and Order — decouple input validation from DB deserialization fallback logic
- **src/api/ApiServer.cpp**: Query-parameter validation for status (Samples, Orders) and priority (Orders) now uses `isValidStatusString`/`isValidPriorityString` instead of try/catch around `stringToStatus`/`stringToPriority` — prevents regression via safe fallback
- **src/utils/CsvImport.cpp**: Status validation during CSV import now checks via `isValidStatusString` before setting — corrupted status in CSV again produces an error message
- **test/unit/test_api.cpp**: Update `SerializeResultJson` assertion on string format of `order_id` (was: numeric, now: `O-YYYY-NNN`)
- **All 181 tests green** (previously: 177/181; 4 regression tests failed)

## [0.8.1] - 2026-05-14

### Security
- **`isApiKeyValid` refactored to be thread-safe**: Now returns `std::optional<std::string>` — eliminates `lastApiKeyRole_` as shared mutable state (race condition prevention)
- **Rate-limiter bypass closed**: Query strings (`?foo=bar`) on the login endpoint bypassed the rate limiter — path is now normalized before comparison
- **RBAC ordering fixed**: User-management routes skip the generic JSON-parse block — auth check now happens before JSON validation (prevents auth-info leak)
- **`verifyPassword` hardened**: `std::stoi` wrapped in try/catch + limit clamped to 1–1,000,000 iterations + salt emptiness check before PBKDF2 call
- **`base64Encode`**: Fix implicit int promotion before left-shift via `static_cast<unsigned int>`

### Bug fixes (Backend)
- **`bindAndListen()`**: FD leak on `bind()`/`listen()` error — `serverFd_` is now closed and reset to `-1` on all error paths
- **`~ApiServer`**: Add missing destructor (RAII cleanup for `serverFd_`); sets `running_ = false` before socket close
- **`createSample`/`createSamplesBatch`**: `updated_at` was written as `0` — now correctly bound via `std::time()` (in-memory object also kept consistent)
- **`isApiKeyValid`**: Erroneous `setError()` on inactive key (key exists but `active=0`) — now calls `clearError()` before `return nullopt`
- **`evaluateFlag()`**: Remove floating-point equality comparison against `0.0` — `referenceHigh_ <= referenceLow_` is sufficient (prevents misclassification at lower bound 0)
- **`stringToStatus`/`stringToFlag`/`stringToPriority`**: Replace `throw std::invalid_argument` on unknown DB strings with safe fallback return values — prevents crash on corrupted rows
- **`CliInterface`**: Mark `canAccessDiagnostics()` and `canAccessSupportData()` as `const`
- **Audit export**: Temporary file was not deleted on error — add `std::remove()` on all error paths; filename now includes PID + atomic sequence counter (collision prevention)

### Bug fixes (Frontend)
- **Sidebar**: `logout()` now navigates to `/login` — prevents brief flash of protected content after logout
- **AuditLog**: Initial fetch result stored in `cancelRef`; `handleApplyFilter`/`handleResetFilter` cancel in-flight requests before new fetch (race condition prevention)
- **Dashboard**: Error from `getDashboardStats()` now surfaced via `console.error` instead of silently swallowed
- **Import — CSV**: Add `csvError` state (`setError` was undefined → TypeScript compile error)
- **Import — all types**: Add 5 MB file-size guard for CSV, HL7, and FHIR
- **Import — all types**: Add `reader.onerror` handler for all `FileReader` instances; size guard now also clears stale filename/content states
- **ResultCreateModal**: Auto-calculated flag no longer overwrites manual selection (new `flagManuallySet` state)

### Tests & build system
- **`test_runner.cpp`**: Remove duplicate `ASSERT_*` macro definitions — now sourced via `#include "test_macros.h"`
- **`test_hl7.cpp` / `test_fhir.cpp`**: Replace hardcoded DB paths with `uniqueDbPath()` using `high_resolution_clock` + atomic counter
- **`CMakeLists.txt`**: `configure_file` writes `version.h` into the build tree (`CMAKE_BINARY_DIR`) instead of the source tree
- **`test/CMakeLists.txt`**: Add `CliInterface.cpp` to test sources; add `CMAKE_BINARY_DIR/include` as explicit include path
- **`test_and_build.sh`**: Fix version number to v0.8; fix binary path to `build/bin/opensylab_tests`

## [0.8.0] - 2026-05-12

### Security (P0)
- **Rate limiting** on `/api/v1/auth/login` (10 requests / 60 seconds per IP, HTTP 429)
- **Forced password change** on first login with default credentials (`admin/admin`)
- **CORS dedup**: `OPENSYLAB_CORS_ORIGIN` is read once in the constructor, no longer twice per request
- **API key RBAC hardened**: Role column in `api_keys` table; keys receive an assigned role instead of hardcoded OPERATOR
- **TLS flags**: `--tls-cert`, `--tls-key` CLI flags + `OPENSYLAB_TLS_CERT/KEY` env vars
- **`--force-https`**: Abort on startup if TLS is not configured — prevents accidental HTTP operation in production

### New features
- **`GET /api/v1/health`**: Unauthenticated health endpoint with version (`{"status":"ok","version":"0.8.0"}`)
- **HL7 v2.5.1 HTTP endpoints**: `POST /api/v1/hl7/import`, `GET /api/v1/hl7/export/{id}` — Hl7Exchange now accessible via HTTP
- **FHIR R4 HTTP endpoints**: `POST /api/v1/fhir/import`, `GET /api/v1/fhir/export/{id}` — FhirExchange now accessible via HTTP
- **Audit log export UI**: Export button on the Audit Log page (CSV, ADMIN only)
- **Status transition validation in backend**: Sample PUT and Order PUT reject invalid status transitions with 422
- **Import page extended**: Tabs for CSV Samples / HL7 v2.5.1 / FHIR R4 import
- **Dashboard statistics precise**: Priority distribution and critical count come from the backend (no longer client-side aggregation with limit:100)
- **Docker healthcheck** enabled: Backend container uses `/api/v1/health`; frontend waits on `service_healthy`
- **CI/CD pipeline** (`.github/workflows/ci.yml`): Backend build+tests and frontend TypeCheck+build on push/PR

### Bug fixes
- Breadcrumb bug: `/audit-log` showed "Dashboard" → corrected to "Audit Log"
- Search: Prefix `O-` instead of `O` for order routing (prevents misrouting on words starting with 'O')
- Create button on Results page no longer visible to VIEWER
- `order_id` in result responses now shows string (`O-2024-001`) instead of numeric FK
- Add `updated_at` to Sample responses
- Remove sidebar badge `'24'` (hardcoded, never rendered)
- TESTING.md: correct 62 tests → 181 tests

### Quality
- TypeScript strict: 0 errors after all changes
- 181 unit tests remain green
- Frontend build: 0 errors, clean

## [0.7.0] - 2026-05-11

### Security
- **JWT authentication** fully implemented (replaces API-key-only mode)
- **PBKDF2-HMAC-SHA256** password hashing (210,000 iterations, OWASP 2023)
- **RBAC** (Role-Based Access Control): ADMIN / OPERATOR / VIEWER / CUSTOM — enforced on all write endpoints
- **Last-admin protection**: updateUser, deleteUser, assignUserRole block demotion/deactivation of the last active admin within a transaction (no TOCTOU window)
- **MFA (TOTP RFC 6238)**: Two-step login flow, ±1 time-window tolerance
- **LDAP authentication** with local shadow account management
- **Self-delete guard**: Admin cannot deactivate their own account

### New features
- **Soft delete** for samples (ARCHIVED), orders (CANCELLED), results (REJECTED) — physical rows kept for audit trail
- **Auto-flag calculation** on results: NORMAL / LOW / HIGH / CRITICAL (margin-based: 50% of reference interval)
- **Batch CSV import** for samples and results with per-row error tracking
- **HL7 v2.5.1 import/export** (ORU^R01) with correct field escaping (`\F\`, `\S\`, …)
- **FHIR R4 Bundle** import/export
- **Audit log export** as CSV with retention policy
- **Dashboard statistics** with status breakdown (real-time data)
- **Single Source of Truth** for version number: `CMakeLists.txt` → `include/version.h` (C++); `package.json` → `import.meta.env.VITE_APP_VERSION` (Frontend)

### Bug fixes (bughunt iterations 25–56)
- 60+ bugs fixed, including:
  - TOCTOU race in updateUser/deleteUser/assignUserRole → transaction opens before read
  - `sqlite3_errmsg()` after `ROLLBACK` → error message now saved before rollback (43 locations)
  - DELETE `/api/v1/users/:id` was unreachable due to routing block
  - `exportValidatedResultsToCsv` TOCTOU: file and audit log now atomic
  - All export functions remove partial files on every error path
  - `makeDbErrorResponse` sanitizes SQLite internals from HTTP responses
  - MFA 403 interceptor was blocking login flow in frontend
  - `ResultEditModal` used numeric PK instead of `result_id` for PUT requests
  - `updateResult` sent wrong field names (`reviewed_by`/`notes` instead of `measured_by`/`comment`)

### Quality
- **181 unit tests** (expanded from 62 to 181)
- TypeScript strict mode — no `any` types in production code
- Full RBAC enforcement in frontend (canWrite guards, sidebar links)
- `useEntityList` hook with correct abort mechanics (no state update after unmount)

## [0.6.0-polish] - 2026-02-11

### New features

#### 🎨 God Mode Dark Theme (Terminal Industrial)
- **Complete Dark Mode Implementation**:
  - Terminal Industrial Aesthetic: 90% "The Abyss" (Deep Navy/Black) + 10% "The Raver" (Neon Accents)
  - Color Palette: Acid Green (#CCFF00), Cyan (#00F0FF), Magenta (#FF0055)
  - Brutalist Tables with square designs (0px border-radius)
  - Hollow HUD-Style Badges with transparent backgrounds
  - Primary Buttons in Biohazard Neon Green with black text
  - Input Fields with Cyan text and Neon Focus Glow
  - Professional Dark Mode Toggle in Profile Page
  - Persistent via localStorage

#### 🎮 Easter Egg: "Ghost in the Machine"
- **Key Sequence Activation**:
  - Secret Code: `s-u-d-o` (outside of input fields)
  - Toggles Dark Mode with CRT glitch effect (300ms)
  - Console messages with neon banners ("SYSTEM OVERRIDE: GOD MODE ENGAGED")
  - Fully invisible in code reviews (disguised as "Keyboard Event Listener")
  - Sync with localStorage for theme persistence

#### ✨ High-Fidelity UI Details
- **Toxic text selection**: Neon green (#CCFF00) highlighting on text selection
- **Industrial scrollbars**: Square precision instruments with Cyan hover state
- **Neon focus glow**: CRT-monitor-like glow effect on focused inputs
- **Scanner line**: Table rows with Acid-Green border (2px left) on hover
- **Instant reactions**: 0s transition for brutal, mechanical feedback

#### ⚛️ Helix Engine Emblem & Reactor Ping
- **Brand Logo Integration**:
  - Helix Engine Emblem placed in Sidebar Header
  - Reactor Ping Animation: 10s heartbeat cycle
  - Asymmetric timing: 10% active (1s flash), 90% silence (9s sleep)
  - Subtle blue glow every 10 seconds (0–5% peak, 5–10% return, 10–100% sleep)
  - "Dangerous Competence" presence signal

#### 📑 Dynamic Document Title Architecture
- **Browser Tab Title Management**:
  - Formula: `[Context] | [Module] | OpenSylab`
  - Examples: `Dashboard | OpenSylab`, `S045 - Edit Sample | OpenSylab`
  - Dirty state support with `*` prefix for unsaved changes
  - useDocumentTitle hook implemented for all pages
  - Automatic cleanup on unmount

#### 🎬 Clinical Transitions (Medical-Grade UI Responsiveness)
- **Modal Animations**:
  - **HUD Snap-In** (Create Forms): 150ms scale(0.97→1) + translateY(10px→0)
  - **Slide from Top** (Edit Forms): 220ms translateY(-30px→0) with bounce easing
  - **Backdrop Fade**: 200ms opacity fade for modal background
- **Page Transitions**:
  - **Data Reveal**: 250ms translateY(4px→0) + opacity fade
  - Content builds up clinically from bottom to top
- **Micro-Interactions**:
  - **Buttons**: 150ms `ease-out` for maximum responsiveness
  - **Table Rows**: 75ms `ease-out` — hover sticks to cursor

#### 🔘 Ghost Button Variant ("Wall of Blood" Fix)
- **New Button Variant**: `ghost`
  - Transparent background with gray text
  - Red only on hover (not permanently solid red)
  - Applied to all table delete buttons (Samples, Orders, Results)
  - Fix for visual noise caused by 20+ red buttons per page

### Improvements

#### 🌐 Frontend UI/UX Polish
- **Professional Dark Mode Toggle**:
  - Removed "God Mode" branding for medical context
  - Clinical description: "Switch to dark theme for reduced eye strain"
  - Standard blue color scheme instead of neon green
  - No status messages or confirmation banners
- **Button Component Optimization**:
  - 150ms ease-out transitions (was 200ms)
  - Instant response feeling for clinical workflows

#### 🔧 Backend Quality Improvements
- **Audit Log API Fixes**:
  - Backend returns English enum values (was German)
  - `actionToString()` and `entityToString()` standardized
  - All entity types (AuditEntry, Order, Sample, TestResult) updated
  - Frontend TypeScript enums now match perfectly

### Technical details

#### New files
- **`frontend/src/context/ThemeContext.tsx`** - Global Dark Mode state management with localStorage
- **`frontend/src/hooks/useDocumentTitle.ts`** - Dynamic browser title hook
- **`frontend/public/assets/brand-helix-core.svg`** - Helix Engine logo (SVG)
- **`frontend/public/assets/brand-helix-core.png`** - Helix Engine logo (PNG, user-provided)

#### Changed files (31 files changed)
- **Backend**: `src/core/AuditEntry.cpp`, `src/core/Order.cpp`, `src/core/Sample.cpp`, `src/core/TestResult.cpp`
- **Frontend Core**: `frontend/index.html` (Easter Egg Script), `frontend/src/index.css` (Dark Mode + Animations)
- **Frontend Components**: All Modals (6), Layout, Sidebar, Button
- **Frontend Pages**: All Pages (8) with Document Title integration

#### CSS animations implemented
- `@keyframes reactor-ping` - Helix Logo heartbeat (10s cycle)
- `@keyframes hud-snap` - Modal create forms (150ms)
- `@keyframes slide-from-top` - Modal edit forms (220ms)
- `@keyframes backdrop-fade` - Modal background (200ms)
- `@keyframes data-reveal` - Page content (250ms)
- `@keyframes glitch` - CRT screen glitch (300ms)

### Statistics
- **31 files changed**
- **1,037 insertions (+)**
- **198 deletions (-)**
- **4 new files** (ThemeContext, useDocumentTitle, 2x logo assets)

### Commit
- **Hash**: `b6a731c`
- **Branch**: `v0.6`
- **Message**: "feat(v0.6): Complete Polish Phase - God Mode Dark Theme, Clinical Transitions & Easter Eggs"

---

## [0.6.0] - 2026-02-02

### New features

#### 🆕 User Management (Admin Interface)
- **User Management Page** for administrators:
  - List of all system users with details (Username, Role, Email, Status, Last Login)
  - Create User Modal with form validation
  - Edit User Modal (Username immutable, optional password update)
  - Delete User with confirmation dialog
  - Role assignment: ADMIN, OPERATOR, VIEWER, CUSTOM
  - Active/Inactive toggle for user accounts
  - Color-coded role badges for visual clarity
- **Backend API endpoints**:
  - GET `/api/v1/users` - List all users (admin only)
  - POST `/api/v1/users` - Create user (admin only)
  - PUT `/api/v1/users/:id` - Update user (admin only)
  - DELETE `/api/v1/users/:id` - Delete user (admin only)
  - GET `/api/v1/users/me` - Get current user profile
  - PUT `/api/v1/users/me/password` - Change password
- **Files**:
  - `frontend/src/pages/Users.tsx` - User Management Page
  - `frontend/src/services/users.ts` - User API service
  - `frontend/src/types/user.ts` - Enhanced user types

#### 🆕 Audit Log Viewer (Compliance & Monitoring)
- **Audit Log Page** for administrators:
  - Complete audit trail of all system actions
  - Multi-criteria filtering (User, Action, Entity, Limit)
  - Adjustable result limit (25, 50, 100, 250 entries)
  - Color-coded action badges (CREATE, UPDATE, DELETE, etc.)
  - Comprehensive table display (Timestamp, User, Action, Entity, Details)
- **Backend API endpoint**:
  - GET `/api/v1/audit` - Get audit log with filters (admin only)
- **Files**:
  - `frontend/src/pages/AuditLog.tsx` - Audit Log Viewer
  - `frontend/src/services/audit.ts` - Audit Log service
  - `frontend/src/types/audit.ts` - Audit entry types

#### 🆕 User Profile & Password Management
- **Profile Page** for all users:
  - Read-only account information display
  - Show Username, Role, Full Name, Email, Account Status
  - Last Login and Account Creation Date
  - User ID display
- **Password Change Functionality**:
  - Secure password change form
  - Current password verification required
  - Password confirmation matching
  - Minimum 8 character requirement
  - Success/error feedback
- **Files**:
  - `frontend/src/pages/Profile.tsx` - User Profile Page

#### 🆕 Enhanced Dashboard Statistics
- **Multi-Entity Statistics Display**:
  - Comprehensive stats for Samples, Orders, Results
  - Status breakdown by entity type
  - Server-side statistics aggregation
  - Real-time data from Stats API
- **Backend API endpoint**:
  - GET `/api/v1/stats` - Get dashboard statistics
- **Files**:
  - `frontend/src/pages/Dashboard.tsx` - Enhanced Dashboard
  - `frontend/src/services/stats.ts` - Statistics service
  - `frontend/src/types/stats.ts` - Statistics types

### Improvements

#### 🔒 Role-Based Access Control (RBAC)
- **Frontend route protection**:
  - Enhanced ProtectedRoute component with `requiredRole` prop
  - Access Denied message for unauthorized access
  - Automatic redirect to Dashboard on missing permissions
- **Backend API protection**:
  - JWT payload role verification
  - Admin-only endpoint enforcement
  - Consistent 403 Forbidden responses
- **Navigation**:
  - Role-based menu filtering in Sidebar
  - Admin-only items hidden for non-admin users
  - New menu items: 👥 Users, 📜 Audit Log, 👤 Profile

#### 📱 UI/UX Improvements
- **Consistent design patterns**:
  - Modal-based forms for Create/Edit operations
  - Confirmation dialogs for destructive actions
  - Color-coded badges for status/roles
  - Responsive table layouts
  - Loading states during async operations
- **Error handling**:
  - Graceful error display
  - API error messages surfaced to UI
  - Form validation feedback
  - Clear user communication

#### 📚 Documentation
- **New documentation files**:
  - `frontend/UI_EXTENSIONS_V06.md` - Comprehensive v0.6 feature guide
  - Default credentials documented (admin/admin)
  - Usage guide for admin and regular users
  - Technical details and file structure

### Security

- **Password security**: Current password verification required for changes
- **Audit logging**: All user actions tracked
- **Role-based access**: Frontend and backend enforcement
- **Session management**: JWT token with expiration

### Technical details

- **Backend**: 8 new API endpoints for User Management, Audit, Stats
- **Frontend**: 3 new pages (Users, AuditLog, Profile)
- **Type safety**: Comprehensive TypeScript types for all entities
- **API integration**: Axios-based services with JWT authentication

### Breaking changes

None — all changes are additive and backward-compatible.

### Default credentials

⚠️ **IMPORTANT**: Change immediately after first login!

- **Username**: `admin`
- **Password**: `admin`

## [0.5.0] - 2026-02-01

### New features

#### 🆕 JWT-based authentication
- **JWT Token Authentication** as replacement for API-key authentication:
  - HS256 algorithm with configurable secret
  - 60-minute token validity
  - Token generation with user claims (userId, username, role)
  - Token validation with signature and expiry check
  - Backward compatibility: JWT-first with API-key fallback
- **Login endpoint**: POST `/api/v1/auth/login`
  - Username/password authentication
  - JWT token response with user info
  - Detailed error handling (401, 400, 500)
- **Library**: jwt-cpp v0.7.0 via CMake FetchContent
- **Files**:
  - `include/auth/JwtAuth.h` - JWT authentication header
  - `src/auth/JwtAuth.cpp` - JWT implementation
  - `src/api/ApiServer.cpp` - Login handler and token validation

#### 🆕 React Frontend (MVP)
- **Single Page Application** with React 18 + TypeScript:
  - Vite build system with Hot Module Replacement
  - TailwindCSS for responsive UI
  - React Router for client-side routing
- **Authentication**:
  - Login page with username/password form
  - JWT token management (localStorage with expiry check)
  - Automatic token validation on app start
  - Protected routes with redirect to login
  - Logout with token cleanup
- **Dashboard**: Overview page with welcome message
- **Sample management** (Samples):
  - Samples list with filtering by status
  - Pagination (20 items per page)
  - Sample Create Modal with form validation
  - Sample Edit Modal with data pre-fill
  - Status dropdown with all SAMPLE_STATUSES
  - Real-time form validation
- **Order management** (Orders):
  - Orders list with dual filtering (Status + Priority)
  - Order Create Modal with full form
  - Order Edit Modal with data preload
  - Status and priority badges with color coding
  - Pagination and responsive table
- **Result management** (Results):
  - Results list with dual filtering (Status + Flag)
  - Display of parameter, value, unit, reference range
  - Flag badges (NORMAL, LOW, HIGH, CRITICAL)
  - Status badges (PENDING, REVIEWED, VALIDATED, etc.)
  - Pagination for large datasets
- **UI components**:
  - Layout with header and navigation (Sidebar with 4 menu items)
  - Card, Button, Input components
  - Loading states and error handling
  - Responsive design
  - Modal-based CRUD workflows
- **API integration**:
  - Axios-based API client
  - JWT Bearer Token interceptor
  - CORS support
  - Error handling with backend message extraction
- **Files**:
  - `frontend/` - Complete React frontend project
  - `frontend/src/services/` - API services (auth, samples, orders, results)
  - `frontend/src/context/AuthContext.tsx` - Auth state management
  - `frontend/src/pages/` - Pages (Login, Dashboard, Samples, Orders, Results)
  - `frontend/src/components/` - UI components (Modals, Common, Layout)

#### 🆕 TLS/HTTPS support
- **OpenSSL integration** for encrypted connections:
  - TLS 1.2+ with modern cipher suites
  - Certificate and private key loading
  - SSL handshake with client connections
  - Optional TLS mode (enable/disable)
- **TlsContext class** for SSL management:
  - OpenSSL initialization
  - SSL context configuration
  - Error handling with detailed messages
- **Dual-mode server**: HTTP or HTTPS depending on configuration
- **Files**:
  - `include/api/TlsContext.h` - TLS context header
  - `src/api/TlsContext.cpp` - TLS implementation
  - `src/api/ApiServer.cpp` - TLS integration in handleClientTls()

#### 🆕 REST API extensions
- **DELETE endpoints** for complete CRUD operations:
  - DELETE `/api/v1/samples/:sample_id` - Delete sample
  - DELETE `/api/v1/orders/:order_id` - Delete order
  - DELETE `/api/v1/results/:result_id` - Delete result
  - Resource existence check before deletion (404 if not found)
  - 204 No Content on successful deletion
  - Audit logging with actor tracking
- **CORS support** for frontend integration:
  - Access-Control-Allow-Origin: http://localhost:5173
  - Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS
  - Access-Control-Allow-Headers: Content-Type, X-API-Key, Authorization
  - OPTIONS preflight handling
- **Improved error handling**:
  - Consistent JSON error responses
  - HTTP status codes following REST conventions
  - Backend error message extraction in frontend

### Improvements

- **Build system**: CMake integration for jwt-cpp and OpenSSL
- **Documentation**:
  - HTTPS_QUICK_START.md - TLS setup guide
  - frontend/README.md - Frontend documentation
  - frontend/DEVELOPMENT.md - Developer guide
  - frontend/QUICK_START.md - Frontend quick start
- **Code quality**:
  - TypeScript for type safety in frontend
  - ESLint configuration
  - Consistent error-handling patterns
- **Security**:
  - JWT token-based authentication
  - TLS/HTTPS encryption
  - Password hashing (DJB2 with salt)
  - Input validation in API and frontend

### Breaking changes

- **Authentication**: API key replaced by JWT tokens (with backward compatibility)
- **Frontend**: New React-based UI instead of CLI-only

### Technical details

- **Frontend stack**: React 18, TypeScript, Vite, TailwindCSS, React Router, Axios
- **Backend dependencies**: jwt-cpp v0.7.0, OpenSSL 3.x
- **API version**: v1 (no breaking changes in existing endpoints)
- **Browser compatibility**: Modern browsers with ES2020+ support

## [0.2.0] - 2026-01-02

### New features

#### 🆕 Order management (Order module)
- **Order data model** with status workflow:
  - Status: REQUESTED → IN_PROGRESS → COMPLETED → VALIDATED → CANCELLED
  - Priority: NORMAL, URGENT, EMERGENCY
  - Link to samples via sampleId
- **CRUD operations** for Orders in Database
- **CLI integration**: Menu items 20–26 for order management
- **Files**:
  - `include/core/Order.h` - Order data model
  - `src/core/Order.cpp` - Implementation
  - `test/unit/test_order.cpp` - Unit tests (8 tests)

#### 🆕 Result entry (TestResult module)
- **TestResult data model** with validation workflow:
  - Status: PENDING → REVIEWED → VALIDATED → REJECTED → AMENDED
  - Flags: NORMAL, ABNORMAL, CRITICAL, INCONCLUSIVE
  - Reference ranges (minValue, maxValue) with automatic flag calculation
- **CRUD operations** for TestResults in Database
- **CLI integration**: Menu items 30–36 for result management
- **Files**:
  - `include/core/TestResult.h` - TestResult data model
  - `src/core/TestResult.cpp` - Implementation
  - `test/unit/test_testresult.cpp` - Unit tests (10 tests)

#### 🆕 Device data interface (CSV result import)
- **CsvResultImport class** for lab device data:
  - Import of analyzer results in CSV format
  - Automatic flag calculation based on reference ranges
  - Link to existing orders
- **Format**: `order_id,parameter,value,unit,min_value,max_value`
- **Fault-tolerant parsing** with detailed statistics
- **Files**:
  - `include/utils/CsvResultImport.h` - Header
  - `src/utils/CsvResultImport.cpp` - Implementation
  - `test/unit/test_csvresultimport.cpp` - Unit tests (5 tests)

#### 🆕 Audit trail (rudimentary)
- **AuditEntry data model** for complete traceability:
  - EntityType: SAMPLE, ORDER, RESULT, USER, SYSTEM
  - ActionType: CREATE, UPDATE, DELETE, VIEW, VALIDATE, LOGIN, LOGOUT
  - Timestamp, user, details
- **Automatic logging** on all CRUD operations
- **CLI integration**: Menu items 50–51 for audit display
- **Files**:
  - `include/core/AuditEntry.h` - AuditEntry data model
  - `src/core/AuditEntry.cpp` - Implementation

#### 🆕 User authentication
- **User data model** with role system:
  - Roles: ADMIN, OPERATOR, VIEWER
  - Active/inactive status
  - Password hashing (DJB2 with salt)
- **Authentication** with login/logout
- **Permission checks** in CLI:
  - Admin: Full access including user management
  - Operator: Create, edit, delete
  - Viewer: Read-only access
- **CLI integration**: Menu items 40–46 for user management
- **Files**:
  - `include/core/User.h` - User data model
  - `src/core/User.cpp` - Implementation

### Critical bug fixes

#### 🔴 HIGH - SQLite Foreign Key Enforcement
- **Problem**: SQLite foreign keys were defined but not enabled
- **Fix**: `PRAGMA foreign_keys = ON` after database connection
- **File**: `src/db/Database.cpp`

### Improvements

- **Test suite extended**: From 18 to 62 tests
- **CLI extended** with 26 new menu items
- **Database schema** extended with 4 new tables (orders, test_results, audit_log, users)
- **Namespace structure** retained (opensylab::core, opensylab::db, opensylab::utils)

## [0.1.1] - 2025-11-25

### Critical bug fixes

#### 🔴 HIGH - Automated tests added
- **Problem**: No automated tests present; missing regression protection
- **Fix**:
  - Simple test framework implemented without external dependencies
  - Unit tests for Sample class (6 tests)
  - Unit tests for Database class (7 tests)
  - Unit tests for CsvImport class (5 tests)
  - Test runner with colored output
  - Total: 18 automated tests
- **Files**:
  - `test/CMakeLists.txt` - Test configuration
  - `test/unit/test_runner.cpp` - Test framework
  - `test/unit/test_sample.cpp` - Sample tests
  - `test/unit/test_database.cpp` - Database tests
  - `test/unit/test_csvimport.cpp` - CSV import tests
  - `test_and_build.sh` - Build & test script

#### 🟡 MEDIUM - Input validation in CLI
- **Problem**: CLI accepts empty/whitespace-only IDs; database contains unusable records
- **Fix**:
  - `readValidatedInput()` function with required-field check
  - `trim()` function removes leading/trailing whitespace
  - `isValidId()` checks for valid characters (alphanumeric, -, _)
  - `isEmpty()` detects empty/whitespace strings
  - User-friendly error messages with retry loop
- **Files**:
  - `include/utils/CliInterface.h` - New validation methods
  - `src/utils/CliInterface.cpp` - Implementation

#### 🟡 MEDIUM - CSV import required-field validation
- **Problem**: CSV import accepts rows without required fields; signals errors only via stderr
- **Fix**:
  - Required-field validation for `sample_id` and `patient_id`
  - Whitespace check with `std::invalid_argument` exception
  - Detailed error statistics (✓ Success / ✗ Error)
  - Clear distinction between "no data" and "error occurred"
  - Error handling with line numbers
- **Files**:
  - `src/utils/CsvImport.cpp` - Improved validation

#### 🟡 MEDIUM - Database::getAllSamples error handling
- **Problem**: On SQL errors an empty vector is returned; CLI incorrectly reports "No samples"
- **Fix**:
  - `hasError()` method for error detection
  - `clearError()` method to reset error state
  - CLI now checks `hasError()` before display
  - Distinction between empty result and error
  - Exception handling when iterating over results
- **Files**:
  - `include/db/Database.h` - New error-handling methods
  - `src/db/Database.cpp` - Improved getAllSamples()
  - `src/utils/CliInterface.cpp` - Error check in handleListSamples() and handleStatistics()

### Improved usability
- Consistent Unicode symbols (✓ ✗ ℹ) for better readability
- Clear error and success messages in all modules
- Improved output with formatting

### Developer tools
- New script: `test_and_build.sh` - Compiles and tests in one step
- Updated `build.sh` - Simplified build process

## [0.1.0] - 2025-11-24

### Initial Release
- Basic project structure
- C++17-based implementation
- SQLite database integration
- CLI interface
- CSV import function
- Sample management (CRUD)
- Modular architecture
- CMake build system
- Basic documentation
