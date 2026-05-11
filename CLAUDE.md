# CLAUDE.md — OpenSylab LIMS

---

## ⚠️ MANDATORY MULTI-MODEL WORKFLOW (STRICT PRECEDENCE)

### 1. Architektur (Claude - The Architect)
- **NO DIRECT CODE WRITING.**
- Maintain system state in `CLAUDE.md`.
- Break down tasks and delegate via `opencode`.
- Preserve token budget for high-level logic.

### 2. Exekution (DeepSeek v4 Flash - The Workhorse)
- Triggered via `opencode`.
- Generates code in **isolated branches**.
- Only receives necessary interfaces/data structures.

### 3. Audit (Linters + Gemini - The Gatekeeper)
- **Static Analysis First:** Code must pass `clang-tidy` with 0 errors before Gemini reads it.
- **Gemini Audit:** Dedicated QA subagent ("Clinical Pathologist") audits against requirements.
- **Anti-Gaslighting:** Ruthless check for edge cases, security, and `CLAUDE.md` compliance.
- **Medical Data:** Extra scrutiny on audit trail completeness and RBAC correctness.

### 4. Merge (Claude)
- Authorized ONLY after Gemini's explicit sign-off.

### 5. Fallback (Claude → DeepSeek+Gemini Review)
- Falls DeepSeek einen Fix nach **3 Versuchen** nicht korrekt umsetzt, übernimmt Claude den Fix direkt.
- Danach **zwingend**: DeepSeek liest den Fix und gibt Feedback, Gemini auditiert — beide müssen APPROVED geben.
- Kein weiteres direktes Code-Schreiben durch Claude bis zum nächsten Fallback.

---

## CHECKLISTE: VOR JEDER AUFGABE

- [ ] `TODO.md` gelesen — existiert das Feature / der Bug bereits?
- [ ] Falls ja: vorhandene Implementierung nutzen, nicht neu schreiben
- [ ] Betrifft die Änderung Audit Trail oder RBAC? → besondere Sorgfalt (ISO 15189)

---

## CHECKLISTE: PRO GEÄNDERTER DATEI

**Code-Qualität (C++17)**
- [ ] Kein `// TODO`, `// FIXME`, `// HACK`, `// placeholder`
- [ ] Kein leerer Funktionsrumpf / Stub (`return {};` ohne Logik)
- [ ] Kein auskommentierter Code der "später" aktiviert werden soll
- [ ] Kein Raw-Pointer-Ownership → `unique_ptr` / `shared_ptr`
- [ ] Keine Magic Numbers → benannte `constexpr`
- [ ] Keine C-Style Casts → `static_cast` / `dynamic_cast` / `reinterpret_cast`
- [ ] Kein `std::cout` / `printf` in Library-Code → Logging via stderr oder dedizierter Logger
- [ ] Alle read-only Parameter `const&`, nicht-mutierende Member-Funktionen `const`
- [ ] Einparameter-Konstruktoren `explicit`
- [ ] RAII für alle Ressourcen (keine nackten `FILE*`, DB-Handles, Sockets)

**Fehlerbehandlung**
- [ ] Öffentliche API-Handler geben HTTP-Fehlercodes zurück, werfen keine Exceptions an Modulgrenzen
- [ ] SQL-Queries immer via Prepared Statements — niemals String-Konkatenation mit User-Input
- [ ] JWT-Validierung schlägt immer fehl-safe: kein Token = kein Zugriff

**Sicherheit (Medical Context)**
- [ ] Jede schreibende Operation auf Labordaten erzeugt einen AuditEntry
- [ ] RBAC-Check vor jedem API-Endpoint der Daten verändert
- [ ] Passwörter/Secrets niemals geloggt, auch nicht debug-zeitig
- [ ] Passwort-Hashing läuft über PBKDF2-HMAC-SHA256 (implementiert) — kein Rückfall auf DJB2

**Build-Registrierung**
- [ ] Neue `.cpp`-Datei → sofort in `CMakeLists.txt` unter dem passenden `*_SOURCES`-Set eintragen
- [ ] Neuer Test → sofort in `test/CMakeLists.txt` unter `TEST_SOURCES` registrieren
- [ ] Neue Layer-Abhängigkeit verletzt nicht die 5-Layer-Hierarchie

**Tests**
- [ ] Jede neue öffentliche Funktion hat mindestens einen Test
- [ ] Test deckt mindestens einen Fehlerpfad ab (nicht nur Happy Path)
- [ ] Security-relevante Funktionen (Auth, RBAC) testen auch den abgelehnten Fall

---

## CHECKLISTE: NACH JEDER ÄNDERUNG

**Build & Test**
- [ ] `cmake --build build --parallel $(nproc) 2>&1 | tee /tmp/sylab_build.log` — 0 Errors, 0 neue Warnings
- [ ] `grep -E "error:|warning:" /tmp/sylab_build.log` — leer
- [ ] `ctest --test-dir build -j$(nproc) --output-on-failure` — alle Tests grün
- [ ] API-Smoke-Test ausgeführt (s.u.) — HTTP 200 auf Health-Endpoint

**Gemini Audit (Gatekeeper)**
- [ ] Agent gestartet mit Auftrag: *"Audit der DeepSeek-Änderungen auf Qualität, Sicherheit, RBAC-Korrektheit und Einhaltung der CLAUDE.md. Antworte APPROVED / NEEDS_FIXES."*
- [ ] Review-Ergebnis ist APPROVED (oder alle Mängel behoben und erneut geprüft)

**Sofortpflicht bei Fund**
- [ ] Jeder entdeckte Fehler (im Review, Log, Build, Test) — egal ob neu oder alt — sofort behoben, bevor weitergemacht wird

---

## Build & Test Kommandos

```bash
# CMake konfigurieren (einmalig)
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build --parallel $(nproc) 2>&1 | tee /tmp/sylab_build.log

# Tests
ctest --test-dir build -j$(nproc) --output-on-failure

# Build-Fehler prüfen
grep -E "error:|warning:" /tmp/sylab_build.log

# Docker (vollständiges System)
docker compose up -d
docker compose logs -f opensylab-backend
```

## API Smoke-Tests

```bash
# Health / Login
curl -s http://localhost:8080/api/v1/health
curl -s -X POST http://localhost:8080/api/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin"}'

# Authentifizierten Request (TOKEN aus Login-Response)
curl -s http://localhost:8080/api/v1/samples \
  -H "Authorization: Bearer $TOKEN"
```

---

## 5-Layer-Architektur

```
Layer 4: src/api/        — HTTP-Endpoints, TLS, Request-Routing (ApiServer, TlsContext)
Layer 3: src/auth/       — JWT Auth, RBAC-Enforcement (JwtAuth)
Layer 2: src/utils/      — CSV-Import, HL7, FHIR, CLI-Interface
Layer 1: src/db/         — SQLite-Persistence (Database)
Layer 0: src/core/       — Domain-Entities (Sample, Order, TestResult, AuditEntry, User)
```

Layer N darf nur Layer N-1 einbinden, nie N+1. Keine Circular Dependencies.  
`src/api/` darf **nicht** direkt `src/db/` importieren — immer über Layer 3/2 durchrouten.

---

## Kanonische Implementierungen

| Konzept | Kanonisch (NUTZEN) |
|---------|-------------------|
| Sample-Persistenz | `src/db/Database.cpp` → `Database::addSample` / `getSamples` |
| Order-Persistenz | `src/db/Database.cpp` → `Database::addOrder` / `getOrders` |
| Audit-Eintrag schreiben | `src/db/Database.cpp` → `Database::addAuditEntry` |
| JWT erstellen/validieren | `src/auth/JwtAuth.cpp` → `JwtAuth::generateToken` / `validateToken` |
| TLS-Kontext | `src/api/TlsContext.cpp` → `TlsContext` |
| CSV-Proben-Import | `src/utils/CsvImport.cpp` |
| CSV-Ergebnis-Import | `src/utils/CsvResultImport.cpp` |
| HL7-Parser | `src/utils/Hl7.cpp` |
| FHIR-Integration | `src/utils/Fhir.cpp` |
| CLI-Interface | `src/utils/CliInterface.cpp` |

---

## Konventionen (Kurzreferenz)

**Fehlerbehandlung:** HTTP-Statuscodes an API-Grenzen · `throw std::runtime_error` intern mit catch im Handler · Kein leeres `catch(...) {}`

**Datenbank:** Immer Prepared Statements (`sqlite3_prepare_v2`) · Transaktionen für Multi-Step-Writes · Foreign Keys aktiviert

**Auth:** Jeder API-Endpoint (außer `/auth/login`) erfordert valides JWT · RBAC-Rollen: `ADMIN > OPERATOR > VIEWER > CUSTOM` · Roles auf Endpoint-Ebene prüfen, nicht nur auf Route-Ebene

**Audit Trail (ISO 15189):** Jede CREATE/UPDATE/DELETE-Operation auf Sample, Order, TestResult, User erzeugt zwingend einen AuditEntry mit `user_id`, `action`, `entity_type`, `entity_id`, `timestamp`

**Frontend (React/TS):** TypeScript strict mode · Keine `any`-Typen · API-Calls nur über zentrale Fetch-Wrapper mit Auth-Header-Injection

---

## Aktive Bugs & Known Issues (Details in `TODO.md`)

Aktive Bugs werden ausschließlich in `TODO.md` dokumentiert.

**Bekannte offene Punkte:**
- Default Credentials `admin/admin` — nur für Dev, niemals in Prod
- TLS vorhanden (`--tls` Flag), aber noch nicht in Production erzwungen

---

## Wichtige Dateipfade

| Was | Pfad |
|-----|------|
| Haupteinstiegspunkt | `src/main.cpp` |
| API-Server | `src/api/ApiServer.{cpp,h}` |
| TLS-Kontext | `src/api/TlsContext.{cpp,h}` |
| JWT Auth | `src/auth/JwtAuth.{cpp,h}` |
| Datenbank | `src/db/Database.{cpp,h}` |
| Domain-Entities | `include/core/*.h` |
| CMake Source-Registrierung | `CMakeLists.txt` (CORE_SOURCES, API_SOURCES, …) |
| **Versionsnummer (C++ SSOT)** | **`CMakeLists.txt` → `project(VERSION x.y.z)` → generiert `include/version.h`** |
| **Versionsnummer (Frontend SSOT)** | **`frontend/package.json` → `"version"` → `import.meta.env.VITE_APP_VERSION`** |
| Versionsdoku | `docs/VERSIONING.md` |
| Test-Registrierung | `test/CMakeLists.txt` |
| Unit-Tests | `test/unit/` |
| Docker-Config | `docker-compose.yml` |
| Offene Bugs / Roadmap | `TODO.md` |
| Design-Guide (Frontend) | `DESIGN.md` |
| Changelog | `CHANGELOG.md` |
