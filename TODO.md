# TODO - OpenSylab v0.7 Roadmap

**Current Version:** v0.7.0 (2026-05-11)
**Branch:** main
**Target:** v0.8.0 Alpha
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

---

### 1. Soft-Delete: Samples [P1]

**Backend**
- [ ] `Database::deleteSample(id)`: SQL von `DELETE` auf `UPDATE samples SET status='ARCHIVED' WHERE id=?` umstellen
- [ ] Rückgabe `false` wenn Sample nicht gefunden (→ 404 im Handler)
- [ ] Rückgabe `false` wenn Sample bereits `ARCHIVED` (→ 409 im Handler)
- [ ] `ApiServer`: DELETE `/api/v1/samples/:id` gibt `204 No Content` bei Erfolg
- [ ] `ApiServer`: DELETE `/api/v1/samples/:id` gibt `404` wenn nicht gefunden
- [ ] `ApiServer`: DELETE `/api/v1/samples/:id` gibt `409` wenn bereits archiviert
- [ ] `ApiServer`: Actor-ID aus JWT-Payload extrahieren und an DB übergeben
- [ ] `Database`: `AuditEntry` mit `ActionType::DELETE`, `EntityType::SAMPLE`, `actor_id`, `entity_id` schreiben

**Frontend**
- [ ] `services/samples.ts`: `deleteSample(id)` auf `DELETE /api/v1/samples/:id` prüfen
- [ ] `Samples.tsx`: `DeleteConfirmDialog` Anbindung verifizieren — ruft `deleteSample()` auf?
- [ ] `Samples.tsx`: Nach erfolgreichem Delete `fetchSamples()` aufrufen (Liste aktualisieren)
- [ ] `Samples.tsx`: Archivierte Samples in der Liste ausgegraut oder ausgeblendet darstellen

---

### 2. Soft-Delete: Orders [P1]

**Backend**
- [ ] `Database::deleteOrder(id)`: `UPDATE orders SET status='CANCELLED' WHERE id=?`
- [ ] Rückgabe `false` wenn nicht gefunden oder bereits `CANCELLED`
- [ ] `ApiServer`: DELETE `/api/v1/orders/:id` — 204 / 404 / 409
- [ ] `ApiServer`: Actor-ID aus JWT extrahieren
- [ ] `Database`: `AuditEntry` mit `ActionType::DELETE`, `EntityType::ORDER` schreiben

**Frontend**
- [ ] `services/orders.ts`: `deleteOrder(id)` prüfen
- [ ] `Orders.tsx`: `DeleteConfirmDialog` verifizieren
- [ ] `Orders.tsx`: Nach Delete Liste aktualisieren

---

### 3. Soft-Delete: Results [P1]

**Backend**
- [ ] `Database::deleteTestResult(id)`: `UPDATE test_results SET status='REJECTED' WHERE id=?`
- [ ] Rückgabe `false` wenn nicht gefunden oder bereits `REJECTED`
- [ ] `ApiServer`: DELETE `/api/v1/results/:id` — 204 / 404 / 409
- [ ] `ApiServer`: Actor-ID aus JWT extrahieren
- [ ] `Database`: `AuditEntry` mit `ActionType::DELETE`, `EntityType::RESULT` schreiben

**Frontend**
- [ ] `services/results.ts`: `deleteResult(id)` prüfen — falls fehlend, ergänzen
- [ ] `Results.tsx`: `DeleteConfirmDialog` anbinden (analog Samples/Orders)
- [ ] `Results.tsx`: Nach Delete Liste aktualisieren

---

### 4. Pagination Fix: Samples [P1]

**Backend**
- [ ] `Database::getSamplesCount(filter)`: neue Methode mit `SELECT COUNT(*) FROM samples WHERE ...`
- [ ] `Database::getSamples(offset, limit, filter)`: `LIMIT ? OFFSET ?` an SQL anhängen
- [ ] `ApiServer`: Query-Parameter `offset` und `limit` aus Request parsen (Defaults: offset=0, limit=20)
- [ ] `ApiServer`: `total` im JSON-Response aus `getSamplesCount()` setzen, nicht `data.size()`

**Frontend**
- [ ] `Samples.tsx`: `SampleListResponse.total` für Seitenberechnung verwenden
- [ ] `Samples.tsx`: `Math.ceil(total / pageSize)` für Gesamtseitenzahl
- [ ] `Samples.tsx`: "Weiter"-Button deaktivieren wenn `offset + pageSize >= total`
- [ ] `Samples.tsx`: Seitenzahl-Anzeige (`Seite X von Y`) ergänzen

---

### 5. Sample-Order-Verknüpfung im Frontend [P1]

**Backend**
- [ ] `ApiServer`: `POST /api/v1/orders` validiert dass `sample_id` in `samples`-Tabelle existiert → `422 Unprocessable Entity` falls nicht

**Frontend**
- [ ] `OrderCreateModal.tsx`: `useEffect` beim Öffnen — alle Samples laden via `getSamples()`
- [ ] `OrderCreateModal.tsx`: Dropdown für `sample_id` mit Sample-IDs + Patientennamen befüllen
- [ ] `OrderCreateModal.tsx`: Typeahead-Filter — Freitext filtert Dropdown-Einträge live
- [ ] `OrderCreateModal.tsx`: Speichern-Button blockiert wenn kein Sample ausgewählt
- [ ] `OrderCreateModal.tsx`: Fehlermeldung wenn keine Samples vorhanden ("Zuerst eine Probe anlegen")
- [ ] `OrderEditModal.tsx`: Verlinktes Sample als read-only Label anzeigen (nicht änderbar)

---

### 6. Result-Order-Verknüpfung + Auto-Flag [P1]

**Backend**
- [ ] `ApiServer`: `POST /api/v1/results` validiert dass `order_id` existiert → `422` falls nicht
- [ ] `ApiServer`: `POST /api/v1/results` prüft Order-Status — nur `IN_PROGRESS` oder `COMPLETED` zulässig → `409` sonst

**Frontend**
- [ ] `ResultCreateModal.tsx`: `useEffect` beim Öffnen — Orders laden via `getOrders()`
- [ ] `ResultCreateModal.tsx`: Dropdown für `order_id` — nur Orders mit Status `IN_PROGRESS` oder `COMPLETED`
- [ ] `ResultCreateModal.tsx`: Auto-Flag-Logik implementieren:
  - `value < reference_min` → Flag `LOW`
  - `value > reference_max` → Flag `HIGH`
  - sonst → Flag `NORMAL`
- [ ] `ResultCreateModal.tsx`: Flag-Preview unterhalb des Wertefelds anzeigen (farbig, vor dem Speichern)
- [ ] `ResultCreateModal.tsx`: Inline-Validierung Referenzbereich — Werte-Input bekommt roten/grünen Border je nach Flag
- [ ] `ResultCreateModal.tsx`: Speichern-Button blockiert wenn kein Order ausgewählt

---

### 7. Status-Workflow-Transitionen [P1]

- [ ] `frontend/src/utils/constants.ts` anlegen (falls nicht vorhanden)
- [ ] `SAMPLE_TRANSITIONS` Map definieren:
  - `REGISTERED` → `[IN_TRANSIT, ARCHIVED]`
  - `IN_TRANSIT` → `[RECEIVED, ARCHIVED]`
  - `RECEIVED` → `[IN_ANALYSIS, ARCHIVED]`
  - `IN_ANALYSIS` → `[COMPLETED, ARCHIVED]`
  - `COMPLETED` → `[ARCHIVED]`
  - `ARCHIVED` → `[]` (keine weiteren Übergänge)
- [ ] `ORDER_TRANSITIONS` Map definieren:
  - `REQUESTED` → `[IN_PROGRESS, CANCELLED]`
  - `IN_PROGRESS` → `[COMPLETED, CANCELLED]`
  - `COMPLETED` → `[VALIDATED, CANCELLED]`
  - `VALIDATED` → `[]`
  - `CANCELLED` → `[]`
- [ ] `SampleEditModal.tsx`: Status-Dropdown filtert nach `SAMPLE_TRANSITIONS[currentStatus]`
- [ ] `OrderEditModal.tsx`: analog mit `ORDER_TRANSITIONS`
- [ ] Ungültige Übergänge sind nicht wählbar (disabled option oder ausgeblendet)

---

### 8. UX: Fetch-Logik deduplizieren [P1]

- [ ] `frontend/src/hooks/useEntityList.ts` erstellen
- [ ] Hook-Signatur: `useEntityList<T>(fetchFn, deps)` → `{ data, loading, error, refetch }`
- [ ] `useCallback` intern verwenden um Race-Conditions zu verhindern
- [ ] `Samples.tsx`: auf `useEntityList` umstellen — `fetchSamples` Duplikate entfernen
- [ ] `Orders.tsx`: auf `useEntityList` umstellen
- [ ] `Results.tsx`: auf `useEntityList` umstellen
- [ ] Manueller Test: Filter schnell wechseln — kein veralteter State sichtbar

---

### 9. UX: Unified Error Handling [P1]

- [ ] `frontend/src/components/common/ErrorBanner.tsx` erstellen
- [ ] Props: `message: string | null`, `onDismiss?: () => void`
- [ ] Styling: roter Hintergrund, Dismiss-Button, Dark-Mode-kompatibel
- [ ] `Samples.tsx`: inline Error-State durch `<ErrorBanner>` ersetzen
- [ ] `Orders.tsx`: analog
- [ ] `Results.tsx`: analog
- [ ] `Users.tsx`: analog
- [ ] `AuditLog.tsx`: analog
- [ ] `Dashboard.tsx`: analog

---

### 10. Dashboard: Charts & Critical-Flag-Kachel [P2]

**Setup**
- [ ] `cd frontend && npm install recharts`
- [ ] TypeScript-Kompatibilität prüfen — `@types/recharts` falls nötig

**Backend**
- [ ] `ApiServer` / `Database`: `GET /api/v1/stats` um `orders.by_priority` ergänzen (COUNT nach `priority`-Feld)
- [ ] `GET /api/v1/stats`: `results.by_flag` ergänzen (COUNT nach `flag`-Feld)

**Frontend**
- [ ] `types/stats.ts`: `by_priority` und `by_flag` in Stats-Typen ergänzen
- [ ] `Dashboard.tsx`: neuer Abschnitt "Visualisierung" unterhalb der Metrikkacheln
- [ ] `BarChart` (Recharts) für Sample-Statusverteilung — Datenquelle: `stats.samples.by_status`
- [ ] `BarChart` für Order-Prioritätsverteilung — Datenquelle: `stats.orders.by_priority`
- [ ] Beide Charts in `ResponsiveContainer` mit `height={200}` wrappen
- [ ] Farben aus `getStatusColor()` / `getPriorityColor()` ableiten
- [ ] Neue Kachel "Kritische Befunde" — Anzahl Results mit `flag === 'CRITICAL'`
- [ ] Kachel-Stil: roter Rahmen + Neon-Akzent im Dark Mode, neutral wenn count === 0

---

### 11. CSV Import UI [P2]

- [ ] `frontend/src/pages/Import.tsx` erstellen
- [ ] Route `/import` in `App.tsx` registrieren
- [ ] Sidebar-Eintrag "Import" hinzufügen (nur ADMIN / OPERATOR sichtbar)
- [ ] `<input type="file" accept=".csv">` mit Drag-and-Drop-Zone
- [ ] `FileReader` API: Datei-Inhalt auslesen
- [ ] `FormData` aufbauen und via `POST /api/v1/samples/import` senden
- [ ] Loading-Spinner während Upload
- [ ] Ergebnis-Tabelle rendern: grüne Zeile = importiert, rote Zeile = Fehler + Grund
- [ ] Gesamtstatistik-Banner: "X von Y Zeilen erfolgreich importiert"
- [ ] "Erneut versuchen" Button nach Fehler (setzt State zurück)
- [ ] Fehlerfall: Backend antwortet mit Fehler-Array pro Zeile — Zeilennummer + Meldung anzeigen

---

### 12. Barcode-Scanner Integration [P2]

- [ ] `frontend/src/hooks/useBarcode.ts` erstellen
- [ ] Feature-Detection: `'BarcodeDetector' in window` — früh prüfen, Ergebnis cachen
- [ ] `navigator.mediaDevices.getUserMedia({ video: { facingMode: 'environment' } })` für Kamera
- [ ] `BarcodeDetector.detect(videoFrame)` in `requestAnimationFrame`-Loop aufrufen
- [ ] Hook gibt `{ scan, isSupported, error }` zurück — `scan()` startet die Kamera
- [ ] `SampleCreateModal.tsx`: Scanner-Icon-Button neben `sample_id`-Input
- [ ] Scanner-Modal: `<video>`-Element für Kamera-Vorschau + Overlay mit Scan-Rahmen
- [ ] Bei erkanntem Code: Modal schließen, `sample_id`-Feld mit Code befüllen
- [ ] Fallback-Hinweis wenn `isSupported === false`: "Scanner nicht verfügbar — bitte manuell eingeben"
- [ ] Kamera-Stream bei Modal-Close stoppen (`stream.getTracks().forEach(t => t.stop())`)

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

---

### Pre-existing: transformSample maps updated_at to registration_date [Compliance]

- [ ] Check backend `sampleToJson()` in `src/api/ApiServer.cpp` — does it send an `updated_at` or `update_date` field?
- [ ] If yes: fix `transformSample` in `frontend/src/services/samples.ts` to use `backendSample.updated_date ?? backendSample.registration_date`
- [ ] ISO 15189 compliance: displaying stale modification timestamps on validated samples is a compliance gap
