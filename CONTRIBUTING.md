# Contributing to OpenSylab

Thank you for your interest in contributing. OpenSylab is a medical LIMS —
correctness, security, and ISO 15189 audit-trail integrity take precedence over
speed. Please read this guide before opening a pull request.

## Ground rules

- Be respectful — see [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).
- **Do not** file security vulnerabilities as public issues — see
  [SECURITY.md](SECURITY.md).
- Discuss substantial changes in an issue before implementing them.

## Project layout (5-layer architecture)

```
Layer 4: src/api/    — HTTP endpoints, TLS, routing
Layer 3: src/auth/   — JWT auth, RBAC enforcement
Layer 2: src/utils/  — CSV/HL7/FHIR, CLI, config
Layer 1: src/db/     — SQLite persistence
Layer 0: src/core/   — domain entities
```

A layer may only depend on the layer directly below it. `src/api/` must not
import `src/db/` directly — route through the `IDatabase` interface. No circular
dependencies.

## Development setup

**Backend (C++17, CMake, OpenSSL, SQLite3):**

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel $(nproc)
ctest --test-dir build --output-on-failure
```

Tests require these environment variables (also set in CI):

```bash
export OPENSYLAB_JWT_SECRET=ci-test-jwt-secret-key-at-least-32-characters-long
export OPENSYLAB_AUDIT_HMAC_KEY=ci-test-audit-hmac-key-at-least-32-characters-long
```

**Frontend (React 19 + TypeScript strict, Vite):**

```bash
cd frontend
npm install
npm run build      # tsc -b && vite build
npm test           # vitest run
npm run lint
```

## Coding conventions

**C++**

- C++17. No `// TODO`/`// FIXME`/`// HACK` or commented-out "later" code in merged PRs.
- RAII for all resources; `unique_ptr`/`shared_ptr` over raw-pointer ownership.
- Named `constexpr` instead of magic numbers; `static_cast`/`dynamic_cast` over C casts.
- `const&` for read-only parameters; `const` non-mutating members; `explicit`
  single-argument constructors.
- No `std::cout`/`printf` in library code — use the `LOG_*` macros (spdlog).
- SQL only via prepared statements (`sqlite3_prepare_v2`) — never string
  concatenation with user input.
- API handlers return HTTP status codes; do not let exceptions cross module boundaries.

**Frontend**

- TypeScript strict mode, no `any`. API calls only through the central fetch
  wrapper with auth-header injection.

**Security & compliance (mandatory)**

- Every CREATE/UPDATE/DELETE on Sample, Order, TestResult, or User **must** write
  an `AuditEntry` (ISO 15189).
- Every data-mutating endpoint enforces an RBAC role check.
- Never log passwords or secrets, even at debug level.

## Registering new files

- New `.cpp` → add to the matching `*_SOURCES` set in `CMakeLists.txt`.
- New test → add to `TEST_SOURCES` in `test/CMakeLists.txt`.

## Tests

- Every new public function needs at least one test.
- Cover at least one failure path, not only the happy path.
- Security-relevant functions (auth, RBAC) must also test the rejected case.

## Pull request checklist

Before opening a PR, confirm:

- [ ] Build is clean: 0 errors, 0 new warnings
- [ ] All tests pass (`ctest` and `npm test`)
- [ ] New public functions are tested, including a failure path
- [ ] Audit-trail and RBAC obligations are met for data-mutating changes
- [ ] `CHANGELOG.md` updated for user-visible changes
- [ ] Version SSOT untouched unless the PR is an intentional release bump
      (`CMakeLists.txt` `project(VERSION …)` and `frontend/package.json`)

CI runs the backend build/tests, frontend build/type-check, `npm audit`, and
CodeQL. All checks must be green before merge.

## Commit & PR style

- Use clear, imperative commit subjects, ideally Conventional Commits
  (`fix(security): …`, `feat(api): …`, `docs: …`).
- Keep PRs focused; one logical change per PR.
- Reference the issue the PR closes.

## License

By contributing, you agree that your contributions are licensed under the
project's [MIT License](LICENSE).
