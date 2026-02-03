# TODO - OpenSylab v0.5 Release Roadmap

**Branch:** v0.5 (Development)
**Target:** v0.5 Alpha Release
**Status:** ~80-85% Complete (TLS + Frontend MVP + CORS Integration Complete)
**Generated:** 2026-01-31
**Last Updated:** 2026-02-01

---

## Priority Levels

- **P0** - Release Blocker (MUST have for v0.5 Alpha)
- **P1** - High Priority (Should have for v0.5 Alpha)
- **P2** - Medium Priority (Nice to have for v0.5, can defer to v0.6)
- **P3** - Low Priority (Future versions)

---

## Release Blockers (P0)

### 🔒 Security & TLS/HTTPS

- [x] **[P0]** ✅ Implement OpenSSL integration for TLS/HTTPS **COMPLETED 2026-02-01**
  - [x] Add OpenSSL dependency to CMakeLists.txt
  - [x] Create TLS context manager class (`TlsContext`)
  - [x] Implement SSL_accept() wrapper in ApiServer
  - [x] Add certificate loading from file/config
  - [x] Support self-signed certificates for development (`scripts/generate_cert.sh`)
  - [x] Add certificate validation for production
  - [x] Update ApiServer::bindAndListen() for TLS socket
  - [x] Update ApiServer::handleClient() for SSL_read/SSL_write
  - **Completed in:** 3 hours (parallel execution)
  - **Documentation:** `TLS_IMPLEMENTATION_REPORT.md`, `HTTPS_QUICK_START.md`

- [ ] **[P0]** Implement JWT-based authentication
  - [ ] Add JWT library dependency (e.g., jwt-cpp)
  - [ ] Create JWT token generation utility
  - [ ] Implement token validation middleware
  - [ ] Add token expiration and refresh logic
  - [ ] Replace API-Key auth with JWT in ApiRouter
  - [ ] Update login endpoint to issue JWT tokens
  - [ ] Add JWT claims (user_id, role, exp)
  - [ ] Implement token blacklist for logout
  - **Estimate:** 3-5 days
  - **Depends on:** jwt-cpp or similar library

### 🌐 Web Frontend (React MVP)

- [x] **[P0]** ✅ Setup React project structure **COMPLETED 2026-02-01**
  - [x] Initialize React app with Vite
  - [x] Setup TypeScript configuration
  - [x] Configure ESLint and Prettier
  - [x] Setup folder structure (components/, pages/, services/, utils/, types/)
  - [x] Add Tailwind CSS for styling
  - [x] Configure API client to backend (localhost:8080)
  - **Completed in:** 2 hours (parallel execution)
  - **Location:** `frontend/`

- [x] **[P0]** ✅ Implement authentication UI **COMPLETED 2026-02-01**
  - [x] Create Login component
  - [x] Create authentication service (login, logout, API key storage)
  - [x] Implement protected route wrapper
  - [x] Add authentication context/provider
  - [x] Create basic layout with logout button
  - **Note:** Using API-Key authentication (JWT deferred to next sprint)
  - **Completed in:** 2 hours (parallel execution)

- [~] **[P0]** Implement Sample Management UI **PARTIALLY COMPLETE**
  - [x] Create SampleList component (table view)
  - [x] Add pagination controls
  - [x] Implement search/filter UI (by status, date)
  - [x] Create SampleDetail display
  - [ ] Add CreateSample form
  - [ ] Add EditSample form
  - [x] Implement API service calls (GET, POST, PUT)
  - [x] Add loading states and error handling
  - [x] Add data transformation layer (backend ↔ frontend format)
  - **Status:** Read-only UI complete, create/edit forms pending
  - **Location:** `frontend/src/pages/Samples.tsx`, `frontend/src/services/samples.ts`

- [x] **[P0]** ✅ Implement Dashboard (minimal) **COMPLETED 2026-02-01**
  - [x] Create Dashboard layout
  - [x] Add statistics cards (total samples, status breakdown)
  - [x] Display recent samples table
  - [x] Add sample status visualization
  - **Completed in:** 2 hours (parallel execution)
  - **Location:** `frontend/src/pages/Dashboard.tsx`

- [x] **[P0]** ✅ Build and deployment configuration **COMPLETED 2026-02-01**
  - [x] Create development build script (`npm run dev`)
  - [x] Create production build script (`npm run build`)
  - [ ] Configure static file serving from ApiServer (deferred)
  - [x] Add CORS headers to API responses
  - [x] Create deployment documentation
  - **Documentation:** `INTEGRATION_COMPLETE.md`, `frontend/INTEGRATION.md`

### 📝 Documentation Updates

- [ ] **[P0]** Update CHANGELOG.md for v0.5
  - [ ] Document all new features (30+ commits)
  - [ ] List API endpoints and methods
  - [ ] Document breaking changes (if any)
  - [ ] Add migration guide from v0.2.0
  - [ ] Credit contributors
  - **Estimate:** 1 day

- [ ] **[P0]** Update README.MD for v0.5
  - [ ] Update feature list (add REST API, Web UI)
  - [ ] Update installation instructions (HTTPS certs, frontend build)
  - [ ] Add API usage examples
  - [ ] Update architecture diagram (add Web Browser → API → DB)
  - [ ] Update technology stack (React, TLS, JWT)
  - **Estimate:** 1 day

- [ ] **[P0]** Bump version to 0.5.0
  - [ ] Update CMakeLists.txt VERSION to 0.5.0
  - [ ] Update main.cpp version string
  - [ ] Update package.json (frontend) version
  - [ ] Tag git commit as v0.5.0
  - **Estimate:** 15 minutes

---

## High Priority (P1)

### 🔧 API Improvements

- [x] **[P1]** ✅ Add CORS support to ApiServer **COMPLETED 2026-02-01**
  - [x] Implement CORS headers in response handlers
  - [x] Add OPTIONS method handler (preflight support)
  - [x] Configure allowed origins (localhost:5173 for development)
  - [x] Add CORS headers to all responses (TLS and Plain HTTP)
  - **Completed in:** 1 hour
  - **Location:** `src/api/ApiServer.cpp:1631-1634, 1709-1712`
  - **Note:** OPTIONS requests handled before authentication check

- [ ] **[P1]** Implement rate limiting
  - [ ] Create rate limiter class (token bucket algorithm)
  - [ ] Add per-IP rate limiting
  - [ ] Add per-API-key rate limiting
  - [ ] Return 429 Too Many Requests on limit exceeded
  - [ ] Make limits configurable
  - **Estimate:** 2 days

- [ ] **[P1]** Add DELETE endpoints
  - [ ] Implement DELETE /api/v1/samples/{id}
  - [ ] Implement DELETE /api/v1/orders/{id}
  - [ ] Implement DELETE /api/v1/results/{id}
  - [ ] Add soft-delete support (mark as archived)
  - [ ] Update tests
  - **Estimate:** 1 day

- [ ] **[P1]** Improve error responses
  - [ ] Standardize error JSON format
  - [ ] Add request_id to error responses
  - [ ] Add detailed validation errors (field-level)
  - [ ] Log errors to audit trail
  - **Estimate:** 1 day

### 📚 API Documentation

- [ ] **[P1]** Generate OpenAPI/Swagger specification
  - [ ] Write OpenAPI 3.0 YAML spec
  - [ ] Document all endpoints (GET, POST, PUT, DELETE)
  - [ ] Add request/response schemas
  - [ ] Add authentication section (JWT)
  - [ ] Add example requests/responses
  - **Estimate:** 3-4 days

- [ ] **[P1]** Setup Swagger UI
  - [ ] Serve Swagger UI from /api/docs
  - [ ] Configure Swagger UI with OpenAPI spec
  - [ ] Add "Try it out" functionality
  - **Estimate:** 1 day
  - **Depends on:** OpenAPI spec

### 🧪 Testing

- [ ] **[P1]** Complete API integration tests
  - [ ] Test all GET endpoints with filters
  - [ ] Test POST endpoints with validation
  - [ ] Test PUT endpoints with conflict detection
  - [ ] Test DELETE endpoints (when implemented)
  - [ ] Test authentication flows (JWT)
  - [ ] Test error cases (400, 401, 404, 409, 500)
  - [ ] Test concurrent requests
  - **Estimate:** 3-4 days

- [ ] **[P1]** Add frontend unit tests
  - [ ] Setup Jest and React Testing Library
  - [ ] Test authentication flow
  - [ ] Test SampleList component
  - [ ] Test form validation
  - [ ] Test API service calls (mocked)
  - **Estimate:** 2-3 days

- [ ] **[P1]** Add E2E tests with Cypress/Playwright
  - [ ] Setup Cypress or Playwright
  - [ ] Test login flow
  - [ ] Test sample CRUD operations
  - [ ] Test dashboard navigation
  - [ ] Test error handling
  - **Estimate:** 3-4 days

### 🏗️ Infrastructure

- [ ] **[P1]** Create development Docker setup
  - [ ] Write Dockerfile for backend (C++ build)
  - [ ] Write Dockerfile for frontend (React build)
  - [ ] Create docker-compose.yml (backend + frontend + db)
  - [ ] Add environment variable configuration
  - [ ] Document Docker usage in README
  - **Estimate:** 2-3 days

- [ ] **[P1]** Add configuration file support
  - [ ] Create opensylab.conf format (INI or YAML)
  - [ ] Add config parser
  - [ ] Support database path, API port, TLS cert paths
  - [ ] Support CORS origins, rate limits
  - [ ] Add --config flag to main.cpp
  - **Estimate:** 2 days

---

## Medium Priority (P2)

### 🐘 PostgreSQL Support

- [ ] **[P2]** Implement PostgreSQL database adapter
  - [ ] Add libpq-dev or libpqxx dependency
  - [ ] Create PostgresDatabase class (inherits from Database interface)
  - [ ] Refactor Database to be abstract base class
  - [ ] Implement all CRUD operations for PostgreSQL
  - [ ] Update schema initialization for PostgreSQL syntax
  - [ ] Add connection pooling
  - [ ] Add database selection via config (sqlite vs postgres)
  - **Estimate:** 1-2 weeks

- [ ] **[P2]** Create database migration system
  - [ ] Design migration file format
  - [ ] Create migration runner
  - [ ] Write migration: v0.2 → v0.5 (SQLite)
  - [ ] Write migration: v0.2 → v0.5 (PostgreSQL)
  - [ ] Add --migrate CLI command
  - [ ] Document migration process
  - **Estimate:** 3-5 days

### 🌐 Frontend Enhancements

- [ ] **[P2]** Add Order Management UI
  - [ ] Create OrderList component
  - [ ] Create OrderDetail component
  - [ ] Add CreateOrder form
  - [ ] Add priority and status badges
  - [ ] Link orders to samples
  - **Estimate:** 3-4 days

- [ ] **[P2]** Add Result Management UI
  - [ ] Create ResultList component
  - [ ] Create ResultDetail component
  - [ ] Add result entry form
  - [ ] Display plausibility flags (colors)
  - [ ] Show reference ranges
  - **Estimate:** 3-4 days

- [ ] **[P2]** Add CSV Import UI
  - [ ] Create file upload component
  - [ ] Show upload progress
  - [ ] Display import results (success/errors)
  - [ ] Add error detail view
  - **Estimate:** 2-3 days

- [ ] **[P2]** Improve Dashboard
  - [ ] Add charts (samples by status, orders over time)
  - [ ] Add filtering controls
  - [ ] Add export functionality
  - **Estimate:** 2-3 days

- [ ] **[P2]** Add responsive design for mobile/tablet
  - [ ] Test on mobile devices
  - [ ] Adjust layout for small screens
  - [ ] Add mobile navigation menu
  - **Estimate:** 2-3 days

### 🔐 Security Enhancements

- [ ] **[P2]** Implement LDAP/Active Directory integration
  - [ ] Add LDAP library dependency
  - [ ] Create LDAP authentication adapter
  - [ ] Add LDAP config to opensylab.conf
  - [ ] Support fallback to local auth
  - [ ] Test with AD server
  - **Estimate:** 4-5 days

- [ ] **[P2]** Add Two-Factor Authentication (2FA)
  - [ ] Implement TOTP (Google Authenticator compatible)
  - [ ] Add 2FA setup UI
  - [ ] Add 2FA verification step in login
  - [ ] Store 2FA secrets securely
  - **Estimate:** 3-4 days

- [ ] **[P2]** Add input sanitization and validation
  - [ ] Add XSS protection (escape HTML in inputs)
  - [ ] Add SQL injection protection (parameterized queries)
  - [ ] Add file upload validation (size, type)
  - [ ] Add Content Security Policy headers
  - **Estimate:** 2-3 days

### 📊 Statistics & Reporting

- [ ] **[P2]** Add graphical statistics to frontend
  - [ ] Integrate Chart.js or Recharts
  - [ ] Create sample status pie chart
  - [ ] Create orders over time line chart
  - [ ] Create results distribution chart
  - **Estimate:** 2-3 days

- [ ] **[P2]** Add PDF report generation
  - [ ] Add PDF library (e.g., libharu)
  - [ ] Create sample report template
  - [ ] Create order report template
  - [ ] Add PDF download endpoint
  - **Estimate:** 3-5 days

### 🧪 Additional Testing

- [ ] **[P2]** Add load testing
  - [ ] Setup k6 or Apache JMeter
  - [ ] Create load test scenarios (API endpoints)
  - [ ] Test concurrent user scenarios
  - [ ] Identify performance bottlenecks
  - [ ] Document performance benchmarks
  - **Estimate:** 2-3 days

- [ ] **[P2]** Add security testing
  - [ ] Run OWASP ZAP against API
  - [ ] Test SQL injection vulnerabilities
  - [ ] Test XSS vulnerabilities
  - [ ] Test authentication bypass attempts
  - [ ] Document and fix findings
  - **Estimate:** 2-3 days

---

## Low Priority (P3)

### 🚀 Performance Optimizations

- [ ] **[P3]** Add caching layer
  - [ ] Implement Redis integration (optional)
  - [ ] Cache frequent queries (sample lists)
  - [ ] Add cache invalidation on updates
  - [ ] Make caching configurable
  - **Estimate:** 3-4 days

- [ ] **[P3]** Optimize database queries
  - [ ] Add indexes on frequently queried columns
  - [ ] Analyze slow query log
  - [ ] Optimize N+1 query issues
  - **Estimate:** 2-3 days

- [ ] **[P3]** Implement async I/O for API server
  - [ ] Refactor to use async sockets (e.g., libuv, Boost.Asio)
  - [ ] Add thread pool for request handling
  - [ ] Test concurrent request performance
  - **Estimate:** 1-2 weeks

### 📱 Mobile App (Future)

- [ ] **[P3]** Design mobile app architecture
  - [ ] Choose framework (React Native, Flutter)
  - [ ] Define mobile-specific features
  - [ ] Create wireframes
  - **Estimate:** 1 week (research/design)

### 🌍 Internationalization

- [ ] **[P3]** Add i18n support to frontend
  - [ ] Setup react-i18next
  - [ ] Extract all UI strings
  - [ ] Add English translations
  - [ ] Add German translations
  - **Estimate:** 3-4 days

- [ ] **[P3]** Add i18n support to API
  - [ ] Add Accept-Language header support
  - [ ] Translate error messages
  - [ ] Support date/time formatting by locale
  - **Estimate:** 2-3 days

### 📦 Additional Features

- [ ] **[P3]** Add user profile management
  - [ ] Create user profile page
  - [ ] Allow password change
  - [ ] Allow email/name updates
  - [ ] Add avatar upload
  - **Estimate:** 2-3 days

- [ ] **[P3]** Add notification system
  - [ ] Design notification model
  - [ ] Add WebSocket support for real-time notifications
  - [ ] Create notification UI component
  - [ ] Add email notifications (optional)
  - **Estimate:** 5-7 days

- [ ] **[P3]** Add advanced search
  - [ ] Full-text search across all entities
  - [ ] Add search suggestions
  - [ ] Add search history
  - **Estimate:** 3-4 days

---

## Deferred to v0.6+

### 🐳 Dockerization (v0.6)

- [ ] Production-ready Docker images
- [ ] Multi-stage Docker builds
- [ ] Docker Compose for production
- [ ] Kubernetes manifests
- [ ] Helm charts
- See: `ROADMAP.MD` v0.6 section

### 🏢 Enterprise Features (v1.0+)

- [ ] Multi-tenancy support
- [ ] Advanced RBAC with custom permissions
- [ ] Compliance reports (ISO 15189, DSGVO)
- [ ] Backup and recovery automation
- [ ] High availability setup
- See: `ROADMAP.MD` v1.0 section

---

## Summary

**P0 (Release Blockers):** 20 tasks → 14 completed ✅, 5 remaining, 1 partial
**P1 (High Priority):** 18 tasks → 1 completed ✅, 17 remaining
**P2 (Medium Priority):** 24 tasks
**P3 (Low Priority):** 13 tasks

**Total:** 75 tasks → 15 completed (20% done)

**Major Milestones Completed (2026-02-01):**
- ✅ TLS/HTTPS backend with OpenSSL
- ✅ React TypeScript frontend with Vite
- ✅ CORS integration (frontend ↔ backend)
- ✅ Authentication flow (API-Key based)
- ✅ Dashboard UI with live data
- ✅ Sample management (read-only)
- ✅ Data transformation layer
- ✅ Integration testing complete

**Remaining for v0.5 Alpha:**
- JWT authentication (replace API-Key)
- Sample create/edit forms
- Documentation updates (CHANGELOG, README)
- Version bump to 0.5.0

**Estimated Time to v0.5 Alpha:**
- Remaining P0 tasks: ~2-3 weeks
- Selected P1 tasks: +1-2 weeks

**Recommended Focus:**
1. ~~Complete P0 tasks first (TLS, Frontend MVP, Documentation)~~ → 70% complete
2. Complete remaining P0: JWT, Sample forms, Documentation
3. Select critical P1 tasks (DELETE endpoints, basic tests)
4. Defer P2/P3 to post-Alpha or v0.6

---

## Notes

- All checkboxes are unchecked for new tasks
- Legacy backlog items (from previous TODO.md) are completed and archived
- Use `git commit` messages to reference TODO items (e.g., "feat(auth): implement JWT [TODO: P0-JWT]")
- Update this file as tasks are completed
- Review and adjust priorities as development progresses

---

**Last Updated:** 2026-02-01 (Integration Milestone)
**Maintainer:** Development Team
**Status:** Active Development (v0.5 branch)
**Parallel Development:** Backend (TLS) + Frontend (React) completed successfully
