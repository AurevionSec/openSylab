# TODO - OpenSylab v0.6 → v0.8 Roadmap

**Current Version:** v0.6.0 + Security Fixes (2026-02-03)
**Branch:** main
**Target:** v0.6.1 or v0.8.0 Alpha
**Status:** ✅ P0 Security Complete - Planning remaining v0.8
**Generated:** 2026-02-03
**Last Updated:** 2026-02-03 (Security fixes documented)

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

## v0.7.0 Planning (Workflow Completeness & UI)

### 🗄️ Backend: API Vollständigkeit (P1)

- [ ] **[P1]** Soft-Delete für Samples implementieren
  - [ ] Backend: `Sample::status` auf `ARCHIVED` setzen statt Row löschen
  - [ ] Backend: `DELETE /api/v1/samples/:id` gibt `204 No Content` zurück
  - [ ] Backend: Audit-Log-Eintrag mit `ActionType::DELETE` + Actor-ID schreiben
  - [ ] Frontend: `deleteSample()` in `services/samples.ts` prüfen (bereits vorhanden)
  - [ ] Frontend: `DeleteConfirmDialog` in `Samples.tsx` bereits angebunden – verifizieren
  - **Estimate:** 1 Tag

- [ ] **[P1]** Soft-Delete für Orders implementieren
  - [ ] Backend: `Order::status` auf `CANCELLED` setzen statt Row löschen
  - [ ] Backend: `DELETE /api/v1/orders/:id` gibt `204 No Content` zurück
  - [ ] Backend: Audit-Log-Eintrag schreiben
  - [ ] Frontend: `deleteOrder()` in `services/orders.ts` prüfen (bereits vorhanden)
  - [ ] Frontend: `DeleteConfirmDialog` in `Orders.tsx` bereits angebunden – verifizieren
  - **Estimate:** 1 Tag

- [ ] **[P1]** Soft-Delete für Results implementieren
  - [ ] Backend: `TestResult::status` auf `REJECTED` setzen statt Row löschen
  - [ ] Backend: `DELETE /api/v1/results/:id` gibt `204 No Content` zurück
  - [ ] Backend: Audit-Log-Eintrag schreiben
  - [ ] Frontend: `deleteResult()` in `services/results.ts` prüfen/hinzufügen
  - [ ] Frontend: `DeleteConfirmDialog` in `Results.tsx` anbinden
  - **Estimate:** 1 Tag

- [ ] **[P1]** `getSamples()` Pagination korrekt implementieren
  - [ ] Backend: `total`-Feld im Response auf echten DB-Count setzen (nicht `data.length`)
  - [ ] Backend: `offset`/`limit` Query-Parameter korrekt an SQLite weiterleiten
  - [ ] Frontend: `SampleListResponse.total` für Pagination in `Samples.tsx` korrekt auswerten
  - **Estimate:** halber Tag

### 🔗 Frontend: Sample-Auftrag-Verknüpfung (P1)

- [ ] **[P1]** Orders mit bestehendem Sample verknüpfen
  - [ ] `OrderCreateModal.tsx`: Dropdown für `sample_id` mit `getSamples()` befüllen
  - [ ] Typeahead-Suche nach Sample-ID im Modal einbauen
  - [ ] Validierung: Order kann nur angelegt werden wenn Sample existiert
  - [ ] `OrderEditModal.tsx`: Verlinktes Sample anzeigen (read-only, da nachträgliche Änderung auditpflichtig)
  - **Estimate:** 1 Tag

- [ ] **[P1]** Results mit Auftrag verknüpfen
  - [ ] `ResultCreateModal.tsx`: Dropdown für `order_id` mit `getOrders()` befüllen
  - [ ] Auftragsstatus vor Ergebniseingabe prüfen (nur `IN_PROGRESS` oder `COMPLETED` zulässig)
  - [ ] Auto-Flag-Berechnung im Frontend: Wenn `value` zwischen `reference_min` und `reference_max` → `NORMAL`, sonst `HIGH`/`LOW`
  - [ ] Referenzbereich-Felder beim Eingeben der Werte visuell validieren (in-line Feedback)
  - **Estimate:** 1,5 Tage

### 📊 Dashboard: Charts & Metriken (P2)

- [ ] **[P2]** Recharts in das Projekt einbinden
  - [ ] `npm install recharts` im `frontend/` Verzeichnis
  - [ ] TypeScript-Typen für Recharts-Props prüfen (`@types/recharts` falls nötig)

- [ ] **[P2]** Samples-Statusverteilung als Chart im Dashboard
  - [ ] `Dashboard.tsx`: Neuer Abschnitt unterhalb der Metrikkacheln
  - [ ] `BarChart` oder `PieChart` aus Recharts mit `stats.samples.by_status` als Datenquelle
  - [ ] Farben aus der bestehenden `getStatusColor()`-Mapping-Funktion ableiten
  - [ ] Responsive: `ResponsiveContainer` mit fixer Höhe (z.B. `200px`)

- [ ] **[P2]** Orders-Prioritätsverteilung als Chart
  - [ ] Separate Zählung nach `priority` (NORMAL / URGENT / EMERGENCY) aus Orders-Daten
  - [ ] `BarChart` neben dem Samples-Chart im gleichen Bento-Grid-Layout

- [ ] **[P2]** Kritische Ergebnisse (Flag: CRITICAL/HIGH) im Dashboard hervorheben
  - [ ] Neue Kachel "Critical Results" mit Anzahl `flag === 'CRITICAL'` aus Results-Daten
  - [ ] Visueller Alert-Stil (roter Rahmen, Neon-Akzent im Dark Mode)
  - **Estimate gesamt:** 2-3 Tage

### 📥 CSV Import: Frontend (P2)

- [ ] **[P2]** CSV-Import Seite/Modal erstellen
  - [ ] Neue Route `/import` oder Modal vom Dashboard aus erreichbar
  - [ ] `<input type="file" accept=".csv">` Komponente
  - [ ] File-Inhalt per FileReader API auslesen und als `FormData` an Backend senden
  - [ ] POST `/api/v1/samples/import` oder existierenden CSV-Endpoint nutzen

- [ ] **[P2]** Import-Fortschritt & Ergebnis anzeigen
  - [ ] Tabelle mit Erfolg/Fehler-Zeilen aus Backend-Response rendern
  - [ ] Farbkodierung: grüne Zeile = importiert, rote Zeile = Fehler mit Grund
  - [ ] Gesamtstatistik: "X von Y Zeilen erfolgreich importiert"
  - [ ] Button "Import wiederholen" nach Fehler
  - **Estimate:** 2 Tage

### 📡 Barcode-Scanner Integration (P2)

- [ ] **[P2]** Web-Barcode-API anbinden
  - [ ] Feature-Detection: `BarcodeDetector` API im Browser prüfen (`'BarcodeDetector' in window`)
  - [ ] Camera-Zugriff via `navigator.mediaDevices.getUserMedia()` anfragen
  - [ ] Neuer React Hook `useBarcode()` in `frontend/src/hooks/useBarcode.ts` erstellen

- [ ] **[P2]** Barcode-Scanner in Sample-Erfassung einbauen
  - [ ] `SampleCreateModal.tsx`: Scanner-Button neben dem `sample_id`-Feld
  - [ ] Scanner-Modal öffnet Kamera-Vorschau, liest Code und befüllt das Textfeld automatisch
  - [ ] Fallback: manuelle Eingabe weiterhin möglich
  - **Estimate:** 2 Tage

### 🔍 UX & Code-Qualität (P1)

- [ ] **[P1]** Refresh-Logik in Pages refactoren
  - [ ] `fetchSamples()`, `fetchOrders()`, `fetchResults()` sind in Samples/Orders/Results je 3× dupliziert
  - [ ] `useCallback` verwenden oder in einen Custom Hook `useEntityList()` extrahieren
  - [ ] Verhindert Race-Conditions bei schnellen Filter-Wechseln

- [ ] **[P1]** Fehlerbehandlung auf allen Pages vereinheitlichen
  - [ ] Wiederverwendbare `<ErrorBanner message={error} />` Komponente erstellen
  - [ ] Alle Pages verwenden aktuell inline-Styles für Error-Darstellung

- [ ] **[P1]** Status-Workflow-Transitionen in den Edit-Dialogen einschränken
  - [ ] `SampleEditModal.tsx`: Nur erlaubte Status-Übergänge im Dropdown anzeigen (z.B. kein ARCHIVED → REGISTERED)
  - [ ] `OrderEditModal.tsx`: Analog für Order-Status
  - [ ] Statusübergänge als Konstante in `utils/constants.ts` definieren
  - **Estimate:** 1 Tag

---

## v0.8.0 Planning (Security & Engineering Stabilization)

### 🔒 Security Enhancements (P0)

- [x] **[P0] ✅ COMPLETED** Replace DJB2 password hashing with PBKDF2/TOTP
  - [x] ✅ Implemented PBKDF2-HMAC-SHA256 (210,000 iterations)
  - [x] ✅ Random 128-bit salt per password
  - [x] ✅ Constant-time comparison (timing-attack resistant)
  - [x] ✅ Backward compatibility for legacy hashes
  - [x] ✅ Replaced DJB2 MFA with RFC 6238 TOTP (HMAC-SHA1)
  - [ ] Add password strength validation UI
  - [ ] Force password reset for existing users (migration)
  - [ ] Update password change UI with strength indicator
  - **Completed:** 2026-02-03
  - **Status:** Production-ready cryptography implemented

- [x] **[P0] ✅ COMPLETED** Implement proper secrets management
  - [x] ✅ JWT secret loaded from OPENSYLAB_JWT_SECRET environment variable
  - [x] ✅ Minimum 32-character validation
  - [x] ✅ Fallback with warning for development
  - [ ] Add config file for additional secrets (opensylab.conf)
  - [ ] Document secret rotation procedures
  - [ ] Add --config flag to load configuration
  - **Completed:** 2026-02-03
  - **Status:** JWT externalization complete

- [x] **[P0] ✅ PARTIAL** HTTPS/TLS Support
  - [x] ✅ TLS support implemented (v0.6.0)
  - [x] ✅ --tls flag with cert/key parameters
  - [ ] Force HTTPS in production mode
  - [ ] Add HTTP → HTTPS redirect
  - [ ] Document certificate setup for production
  - [ ] Add Let's Encrypt integration guide
  - **Status:** TLS available, enforcement optional


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

## Deferred to v0.9+

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

**v0.6.1/v0.7.0 Security Update (COMPLETED 2026-02-03):**
- ✅ PBKDF2-HMAC-SHA256 password hashing (production-ready)
- ✅ RFC 6238 TOTP for MFA (HMAC-SHA1, industry-standard)
- ✅ JWT secret externalization (environment variable)
- ✅ All P0 critical security vulnerabilities resolved

**v0.7.0 Remaining Focus:**
- 🚀 Complete CRUD operations in frontend
- 📊 Data visualization and CSV import UI
- 📱 Mobile responsive design & advanced filtering

**v0.8.0 Remaining Focus:**
- 🐳 Docker containerization for production
- 📚 OpenAPI documentation
- 🧪 Comprehensive testing & CI/CD pipeline
- 🔒 Security Hardening

**Estimated Timeline:**
- v0.7.0 Release: 2-3 weeks
- v0.8.0 Alpha: +3-4 weeks
- v0.9.0 Beta: +4-6 weeks
- v1.0.0 Production: +8-12 weeks

**Current Development Status:**
- **v0.6.0:** ✅ RELEASED (2026-02-03)
- **v0.7.0:** 🏗️ IN PROGRESS
- **v0.8.0:** 📋 PLANNING
- **Branch:** main (v0.6 merged)
- **Contributors:** Development Team + Claude Code + Happy

---

## Notes

### ✅ Security Status (Updated 2026-02-03)

- **Password Hashing:** ✅ PBKDF2-HMAC-SHA256 (production-ready)
- **MFA/TOTP:** ✅ RFC 6238 HMAC-SHA1 (production-ready)
- **JWT Secret:** ✅ Environment variable (`OPENSYLAB_JWT_SECRET`)
- **TLS/HTTPS:** ✅ Available (optional enforcement)

### ⚠️ Production Deployment Requirements

- Default credentials (admin/admin) MUST be changed
- Set `OPENSYLAB_JWT_SECRET` environment variable (min 64 chars recommended)
- Enable TLS with `--tls --tls-cert cert.pem --tls-key key.pem`
- Force existing users to change passwords (migrates to PBKDF2)
- Update this file as tasks are completed
- Use semantic versioning for all releases

---

**Last Updated:** 2026-02-03 (Security fixes completed)
**Maintainer:** Development Team
**Status:** v0.6.0 + Security Fixes → Ready for v0.7.0
**Security:** ✅ Production-ready cryptography implemented
**Next Milestone:** Complete CRUD + Frontend enhancements
