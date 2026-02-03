# TODO - OpenSylab v0.6 → v0.7 Roadmap

**Current Version:** v0.6.0 (Released 2026-02-03)
**Branch:** main
**Target:** v0.7.0 Alpha
**Status:** v0.6.0 Complete ✅ - Planning v0.7
**Generated:** 2026-02-03
**Last Updated:** 2026-02-03

---

## v0.6.0 Release Summary ✅

**Released:** 2026-02-03
**Tag:** v0.6.0
**Commit:** aec85df

### Completed Features

- ✅ **Complete React Frontend Application**
  - React 18 + TypeScript + Vite
  - Tailwind CSS styling
  - React Router navigation
  - Full CRUD operations (Samples, Orders, Results)

- ✅ **User Management System**
  - Admin-only CRUD interface for users
  - Role-based access control (ADMIN, OPERATOR, VIEWER, CUSTOM)
  - User profile management
  - Password change functionality
  - 8 new API endpoints

- ✅ **Audit Log & Compliance**
  - Complete audit trail for all operations
  - Admin-only audit log viewer
  - Filter by user, action, entity, date range
  - Compliance tracking for ISO 15189

- ✅ **Enhanced Dashboard**
  - Multi-entity statistics (Samples, Orders, Results)
  - Status breakdowns with counts
  - Real-time data from backend
  - Server-side aggregation

- ✅ **JWT Authentication**
  - JWT-based authentication with jwt-cpp
  - Token expiration (1 hour)
  - Role verification in JWT payload
  - Secure password hashing (DJB2 with salt)

- ✅ **Documentation**
  - Complete UI_EXTENSIONS_V06.md guide
  - Updated CHANGELOG.md with v0.6.0 notes
  - Default credentials documented
  - Usage guides for admins and users

### Statistics
- **22 new files**
- **6,436 insertions**
- **38 deletions**
- **8 new API endpoints**
- **3 new UI pages** (Users, Audit Log, Profile)
- **Version bump:** 0.2.0 → 0.6.0

---

## Priority Levels

- **P0** - Critical (MUST have for v0.7)
- **P1** - High Priority (Should have for v0.7)
- **P2** - Medium Priority (Nice to have, can defer to v0.8)
- **P3** - Low Priority (Future versions)

---

## v0.7.0 Planning (Next Release)

### 🔒 Security Enhancements (P0)

- [ ] **[P0]** Replace DJB2 password hashing with bcrypt/argon2
  - [ ] Add bcrypt library dependency
  - [ ] Update User::hashPassword() implementation
  - [ ] Add password strength validation
  - [ ] Migrate existing password hashes
  - [ ] Update password change UI with strength indicator
  - **Estimate:** 2-3 days
  - **Security:** Current DJB2 is NOT production-ready

- [ ] **[P0]** Implement proper secrets management
  - [ ] Move JWT secret to environment variable
  - [ ] Add config file for secrets (opensylab.conf)
  - [ ] Document secret rotation procedures
  - [ ] Add --config flag to load configuration
  - **Estimate:** 1-2 days

- [ ] **[P0]** Add HTTPS enforcement
  - [ ] Force HTTPS in production mode
  - [ ] Add HTTP → HTTPS redirect
  - [ ] Document certificate setup for production
  - [ ] Add Let's Encrypt integration guide
  - **Estimate:** 2 days

### 🚀 Core Features (P1)

- [ ] **[P1]** Complete Sample CRUD in Frontend
  - [x] Sample list view
  - [x] Sample detail view
  - [ ] Create sample modal/form
  - [ ] Edit sample modal/form
  - [ ] Delete confirmation dialog
  - [ ] Barcode scanning integration
  - **Estimate:** 3-4 days

- [ ] **[P1]** Complete Order Management UI
  - [x] Order list view
  - [x] Order detail view
  - [ ] Create order form
  - [ ] Edit order form
  - [ ] Link orders to samples
  - [ ] Status workflow transitions
  - **Estimate:** 3-4 days

- [ ] **[P1]** Complete Result Management UI
  - [x] Result list view
  - [x] Result detail view
  - [ ] Result entry form
  - [ ] Validation workflow
  - [ ] Plausibility checking
  - [ ] Reference range validation
  - **Estimate:** 4-5 days

- [ ] **[P1]** Add DELETE endpoints to API
  - [ ] DELETE /api/v1/samples/:id
  - [ ] DELETE /api/v1/orders/:id
  - [ ] DELETE /api/v1/results/:id
  - [ ] Implement soft-delete (mark as archived)
  - [ ] Add audit log entries for deletes
  - [ ] Update frontend to use DELETE
  - **Estimate:** 2 days

### 🐳 Infrastructure (P1)

- [ ] **[P1]** Production Docker setup
  - [ ] Multi-stage Dockerfile for backend
  - [ ] Dockerfile for frontend (nginx)
  - [ ] Docker Compose for production
  - [ ] Environment variable configuration
  - [ ] Volume mounts for data persistence
  - [ ] Health check endpoints
  - **Estimate:** 3-4 days

- [ ] **[P1]** Configuration file system
  - [ ] Define opensylab.conf format (YAML/INI)
  - [ ] Add config parser (libconfig++ or yaml-cpp)
  - [ ] Support: database path, API port, TLS certs, CORS origins
  - [ ] Add --config CLI flag
  - [ ] Document all configuration options
  - **Estimate:** 2-3 days

### 📚 Documentation (P1)

- [ ] **[P1]** API Documentation
  - [ ] Create OpenAPI 3.0 specification
  - [ ] Document all 30+ endpoints
  - [ ] Add request/response schemas
  - [ ] Add authentication documentation
  - [ ] Setup Swagger UI at /api/docs
  - **Estimate:** 3-4 days

- [ ] **[P1]** Production Deployment Guide
  - [ ] Server requirements (CPU, RAM, disk)
  - [ ] Installation steps for Ubuntu/Debian
  - [ ] HTTPS certificate setup
  - [ ] Database backup procedures
  - [ ] Monitoring and logging setup
  - [ ] Security hardening checklist
  - **Estimate:** 2-3 days

### 🧪 Testing (P1)

- [ ] **[P1]** Integration tests for new endpoints
  - [ ] Test user management CRUD
  - [ ] Test audit log filtering
  - [ ] Test statistics endpoints
  - [ ] Test role-based access control
  - [ ] Test password change flow
  - **Estimate:** 2-3 days

- [ ] **[P1]** Frontend unit tests
  - [ ] Setup Jest + React Testing Library
  - [ ] Test authentication context
  - [ ] Test protected routes
  - [ ] Test user management components
  - [ ] Test audit log components
  - [ ] Test profile page
  - **Estimate:** 3-4 days

---

## Medium Priority (P2)

### 🌐 Frontend Enhancements

- [ ] **[P2]** Add data visualization
  - [ ] Integrate Chart.js or Recharts
  - [ ] Sample status pie chart
  - [ ] Orders over time line chart
  - [ ] Results distribution charts
  - **Estimate:** 3-4 days

- [ ] **[P2]** CSV Import UI
  - [ ] File upload component
  - [ ] Progress indicator
  - [ ] Import results display
  - [ ] Error handling and reporting
  - **Estimate:** 2-3 days

- [ ] **[P2]** Mobile responsive design
  - [ ] Test on mobile devices
  - [ ] Responsive navigation menu
  - [ ] Touch-friendly UI components
  - [ ] Mobile-optimized tables
  - **Estimate:** 3-4 days

- [ ] **[P2]** Advanced filtering
  - [ ] Multi-field filter builder
  - [ ] Save filter presets
  - [ ] Export filtered data
  - **Estimate:** 2-3 days

### 🐘 Database Improvements

- [ ] **[P2]** PostgreSQL support
  - [ ] Add libpqxx dependency
  - [ ] Create PostgresDatabase adapter
  - [ ] Refactor Database to abstract base class
  - [ ] Implement connection pooling
  - [ ] Add database selection in config
  - **Estimate:** 1-2 weeks

- [ ] **[P2]** Database migrations
  - [ ] Design migration file format
  - [ ] Create migration runner
  - [ ] Write migrations: v0.2 → v0.6
  - [ ] Add --migrate CLI command
  - **Estimate:** 4-5 days

### 🔐 Advanced Security

- [ ] **[P2]** Two-Factor Authentication
  - [ ] TOTP implementation (Google Authenticator)
  - [ ] 2FA setup UI
  - [ ] QR code generation
  - [ ] Backup codes
  - **Estimate:** 4-5 days

- [ ] **[P2]** LDAP/Active Directory
  - [ ] Add LDAP library
  - [ ] LDAP authentication adapter
  - [ ] Fallback to local auth
  - [ ] User sync from LDAP
  - **Estimate:** 5-6 days

- [ ] **[P2]** Rate limiting
  - [ ] Token bucket algorithm
  - [ ] Per-IP rate limiting
  - [ ] Per-user rate limiting
  - [ ] Return 429 on limit exceeded
  - **Estimate:** 2-3 days

### 📊 Reporting

- [ ] **[P2]** PDF report generation
  - [ ] Add PDF library (libharu)
  - [ ] Sample report template
  - [ ] Order report template
  - [ ] Result report with graphs
  - [ ] PDF download endpoints
  - **Estimate:** 4-5 days

- [ ] **[P2]** Excel/CSV export
  - [ ] Export samples to CSV/Excel
  - [ ] Export orders to CSV/Excel
  - [ ] Export results to CSV/Excel
  - [ ] Export audit log to CSV
  - **Estimate:** 2-3 days

---

## Low Priority (P3)

### 🚀 Performance

- [ ] **[P3]** Caching layer
  - [ ] Redis integration (optional)
  - [ ] Cache frequent queries
  - [ ] Cache invalidation strategy
  - **Estimate:** 3-4 days

- [ ] **[P3]** Async I/O
  - [ ] Refactor to libuv or Boost.Asio
  - [ ] Thread pool for requests
  - [ ] Performance benchmarks
  - **Estimate:** 1-2 weeks

### 🌍 Internationalization

- [ ] **[P3]** Frontend i18n
  - [ ] Setup react-i18next
  - [ ] Extract all strings
  - [ ] English translations
  - [ ] German translations
  - **Estimate:** 3-4 days

- [ ] **[P3]** API i18n
  - [ ] Accept-Language header support
  - [ ] Translate error messages
  - [ ] Locale-specific date/time formatting
  - **Estimate:** 2-3 days

### 📱 Additional Features

- [ ] **[P3]** Notification system
  - [ ] WebSocket support
  - [ ] Real-time notifications UI
  - [ ] Email notifications (optional)
  - **Estimate:** 5-7 days

- [ ] **[P3]** Advanced search
  - [ ] Full-text search
  - [ ] Search suggestions
  - [ ] Search history
  - **Estimate:** 3-4 days

- [ ] **[P3]** Barcode integration
  - [ ] Barcode scanner support
  - [ ] Barcode generation
  - [ ] Label printing
  - **Estimate:** 3-4 days

---

## Deferred to v0.8+

### 🏢 Enterprise Features (v1.0+)

- [ ] Multi-tenancy support
- [ ] Advanced RBAC with custom permissions
- [ ] Compliance reports (ISO 15189, GDPR)
- [ ] Backup automation
- [ ] High availability setup
- [ ] Kubernetes deployment
- [ ] SSO integration (SAML, OAuth)

### 📱 Mobile App (Future)

- [ ] React Native or Flutter app
- [ ] Mobile-specific workflows
- [ ] Offline mode support
- [ ] Push notifications

---

## Summary

**v0.6.0 Achievements:**
- ✅ Complete React frontend with TypeScript
- ✅ User management with RBAC
- ✅ Audit logging for compliance
- ✅ Enhanced dashboard with statistics
- ✅ JWT authentication
- ✅ Professional LIMS UI/UX

**v0.7.0 Focus:**
- 🔒 Production-ready security (bcrypt, secrets management, HTTPS)
- 🚀 Complete CRUD operations in frontend
- 🐳 Docker containerization for production
- 📚 OpenAPI documentation
- 🧪 Comprehensive testing

**Estimated Timeline:**
- v0.7.0 Alpha: 4-6 weeks
- v0.8.0 Beta: +6-8 weeks
- v1.0.0 Production: +8-12 weeks

**Current Development Status:**
- **v0.6.0:** ✅ RELEASED (2026-02-03)
- **v0.7.0:** 🏗️ PLANNING
- **Branch:** main (v0.6 merged)
- **Contributors:** Development Team + Claude Code + Happy

---

## Notes

- Default credentials (admin/admin) MUST be changed in production
- Current password hashing (DJB2) is NOT production-ready - use for development only
- JWT secret is hardcoded - MUST be externalized for production
- All P0 security tasks should be completed before production deployment
- Update this file as tasks are completed
- Use semantic versioning for all releases

---

**Last Updated:** 2026-02-03 (v0.6.0 Release)
**Maintainer:** Development Team
**Status:** Planning v0.7.0
**Next Milestone:** Production-ready security + Complete CRUD
