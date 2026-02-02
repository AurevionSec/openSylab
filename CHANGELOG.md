# OpenSylab - Changelog

Alle wichtigen Änderungen an diesem Projekt werden in dieser Datei dokumentiert.

## [0.6.0] - 2026-02-02

### Neue Features

#### 🆕 User Management (Admin Interface)
- **User Management Page** für Administratoren:
  - Liste aller Systembenutzer mit Details (Username, Role, Email, Status, Last Login)
  - Create User Modal mit Formularvalidierung
  - Edit User Modal (Username immutable, optionales Passwort-Update)
  - Delete User mit Bestätigungsdialog
  - Role Assignment: ADMIN, OPERATOR, VIEWER, CUSTOM
  - Active/Inactive Toggle für Benutzerkonten
  - Color-coded Role Badges für visuelle Klarheit
- **Backend API Endpoints**:
  - GET `/api/v1/users` - List all users (admin only)
  - POST `/api/v1/users` - Create user (admin only)
  - PUT `/api/v1/users/:id` - Update user (admin only)
  - DELETE `/api/v1/users/:id` - Delete user (admin only)
  - GET `/api/v1/users/me` - Get current user profile
  - PUT `/api/v1/users/me/password` - Change password
- **Dateien**:
  - `frontend/src/pages/Users.tsx` - User Management Page
  - `frontend/src/services/users.ts` - User API Service
  - `frontend/src/types/user.ts` - Enhanced User Types

#### 🆕 Audit Log Viewer (Compliance & Monitoring)
- **Audit Log Page** für Administratoren:
  - Vollständiger Audit Trail aller Systemaktionen
  - Multi-Criteria Filtering (User, Action, Entity, Limit)
  - Adjustable Result Limit (25, 50, 100, 250 entries)
  - Color-coded Action Badges (CREATE, UPDATE, DELETE, etc.)
  - Comprehensive Table Display (Timestamp, User, Action, Entity, Details)
- **Backend API Endpoint**:
  - GET `/api/v1/audit` - Get audit log with filters (admin only)
- **Dateien**:
  - `frontend/src/pages/AuditLog.tsx` - Audit Log Viewer
  - `frontend/src/services/audit.ts` - Audit Log Service
  - `frontend/src/types/audit.ts` - Audit Entry Types

#### 🆕 User Profile & Password Management
- **Profile Page** für alle Benutzer:
  - Read-only Account Information Display
  - Show Username, Role, Full Name, Email, Account Status
  - Last Login und Account Creation Date
  - User ID Display
- **Password Change Functionality**:
  - Secure Password Change Form
  - Current Password Verification Required
  - Password Confirmation Matching
  - Minimum 8 Character Requirement
  - Success/Error Feedback
- **Dateien**:
  - `frontend/src/pages/Profile.tsx` - User Profile Page

#### 🆕 Enhanced Dashboard Statistics
- **Multi-Entity Statistics Display**:
  - Comprehensive Stats für Samples, Orders, Results
  - Status Breakdown by Entity Type
  - Server-side Statistics Aggregation
  - Real-time Data from Stats API
- **Backend API Endpoint**:
  - GET `/api/v1/stats` - Get dashboard statistics
- **Dateien**:
  - `frontend/src/pages/Dashboard.tsx` - Enhanced Dashboard
  - `frontend/src/services/stats.ts` - Statistics Service
  - `frontend/src/types/stats.ts` - Statistics Types

### Verbesserungen

#### 🔒 Role-Based Access Control (RBAC)
- **Frontend Route Protection**:
  - Enhanced ProtectedRoute Component mit `requiredRole` Prop
  - Access Denied Message für unauthorized Access
  - Automatic Redirect zu Dashboard bei fehlenden Berechtigungen
- **Backend API Protection**:
  - JWT Payload Role Verification
  - Admin-only Endpoints enforcement
  - Consistent 403 Forbidden Responses
- **Navigation**:
  - Role-based Menu Filtering in Sidebar
  - Admin-only Items hidden für Non-Admin Users
  - New Menu Items: 👥 Users, 📜 Audit Log, 👤 Profile

#### 📱 UI/UX Improvements
- **Consistent Design Patterns**:
  - Modal-based Forms für Create/Edit Operations
  - Confirmation Dialogs für Destructive Actions
  - Color-coded Badges für Status/Roles
  - Responsive Table Layouts
  - Loading States während Async Operations
- **Error Handling**:
  - Graceful Error Display
  - API Error Messages surfaced to UI
  - Form Validation Feedback
  - Clear User Communication

#### 📚 Documentation
- **New Documentation Files**:
  - `frontend/UI_EXTENSIONS_V06.md` - Comprehensive v0.6 Feature Guide
  - Default Credentials Documented (admin/admin)
  - Usage Guide für Admin und Regular Users
  - Technical Details und File Structure

### Sicherheit

- **Password Security**: Current Password Verification für Changes
- **Audit Logging**: All User Actions Tracked
- **Role-Based Access**: Frontend und Backend Enforcement
- **Session Management**: JWT Token with Expiration

### Technische Details

- **Backend**: 8 neue API Endpoints für User Management, Audit, Stats
- **Frontend**: 3 neue Pages (Users, AuditLog, Profile)
- **Type Safety**: Comprehensive TypeScript Types für alle Entities
- **API Integration**: Axios-based Services mit JWT Authentication

### Breaking Changes

Keine - alle Änderungen sind additiv und rückwärtskompatibel.

### Default Credentials

⚠️ **IMPORTANT**: Change immediately after first login!

- **Username**: `admin`
- **Password**: `admin`

## [0.5.0] - 2026-02-01

### Neue Features

#### 🆕 JWT-basierte Authentifizierung
- **JWT Token Authentication** als Ersatz für API-Key-Authentifizierung:
  - HS256-Algorithmus mit konfigurierbarem Secret
  - 60-Minuten Token-Gültigkeit
  - Token-Generierung mit User-Claims (userId, username, role)
  - Token-Validierung mit Signatur- und Ablaufprüfung
  - Rückwärtskompatibilität: JWT-first mit API-Key-Fallback
- **Login-Endpoint**: POST `/api/v1/auth/login`
  - Username/Password-Authentifizierung
  - JWT-Token-Rückgabe mit User-Info
  - Detaillierte Fehlerbehandlung (401, 400, 500)
- **Bibliothek**: jwt-cpp v0.7.0 via CMake FetchContent
- **Dateien**:
  - `include/auth/JwtAuth.h` - JWT-Authentifizierung Header
  - `src/auth/JwtAuth.cpp` - JWT-Implementierung
  - `src/api/ApiServer.cpp` - Login-Handler und Token-Validierung

#### 🆕 React Frontend (MVP)
- **Single Page Application** mit React 18 + TypeScript:
  - Vite Build-System mit Hot Module Replacement
  - TailwindCSS für responsive UI
  - React Router für Client-Side-Routing
- **Authentifizierung**:
  - Login-Seite mit Username/Password-Formular
  - JWT-Token-Management (localStorage mit Ablaufprüfung)
  - Automatische Token-Validierung bei App-Start
  - Protected Routes mit Redirect zu Login
  - Logout-Funktion mit Token-Bereinigung
- **Dashboard**: Übersichtsseite mit Willkommensnachricht
- **Probenverwaltung** (Samples):
  - Samples-Liste mit Filtering nach Status
  - Pagination (20 Items pro Seite)
  - Sample Create Modal mit Formularvalidierung
  - Sample Edit Modal mit Datenvorausfüllung
  - Status-Dropdown mit allen SAMPLE_STATUSES
  - Real-time Formularvalidierung
- **Auftragsverwaltung** (Orders):
  - Orders-Liste mit Dual-Filtering (Status + Priority)
  - Order Create Modal mit vollständigem Formular
  - Order Edit Modal mit Datenvorladung
  - Status- und Priority-Badges mit Farbcodierung
  - Pagination und Responsive Table
- **Ergebnisverwaltung** (Results):
  - Results-Liste mit Dual-Filtering (Status + Flag)
  - Anzeige von Parameter, Value, Unit, Reference Range
  - Flag-Badges (NORMAL, LOW, HIGH, CRITICAL)
  - Status-Badges (PENDING, REVIEWED, VALIDATED, etc.)
  - Pagination für große Datenmengen
- **UI-Komponenten**:
  - Layout mit Header und Navigation (Sidebar mit 4 Menüpunkten)
  - Card, Button, Input Komponenten
  - Loading States und Error Handling
  - Responsive Design
  - Modal-basierte CRUD-Workflows
- **API-Integration**:
  - Axios-basierte API-Client
  - JWT Bearer Token Interceptor
  - CORS-Unterstützung
  - Fehlerbehandlung mit Backend-Message-Extraktion
- **Dateien**:
  - `frontend/` - Komplettes React-Frontend-Projekt
  - `frontend/src/services/` - API Services (auth, samples, orders, results)
  - `frontend/src/context/AuthContext.tsx` - Auth-State-Management
  - `frontend/src/pages/` - Seiten (Login, Dashboard, Samples, Orders, Results)
  - `frontend/src/components/` - UI-Komponenten (Modals, Common, Layout)

#### 🆕 TLS/HTTPS-Unterstützung
- **OpenSSL-Integration** für verschlüsselte Verbindungen:
  - TLS 1.2+ mit modernen Cipher Suites
  - Zertifikat- und Private-Key-Loading
  - SSL-Handshake mit Client-Verbindungen
  - Optionaler TLS-Modus (aktivierbar/deaktivierbar)
- **TlsContext-Klasse** für SSL-Management:
  - OpenSSL-Initialisierung
  - SSL-Kontext-Konfiguration
  - Fehlerbehandlung mit detaillierten Messages
- **Dual-Mode-Server**: HTTP oder HTTPS je nach Konfiguration
- **Dateien**:
  - `include/api/TlsContext.h` - TLS-Kontext Header
  - `src/api/TlsContext.cpp` - TLS-Implementierung
  - `src/api/ApiServer.cpp` - TLS-Integration in handleClientTls()

#### 🆕 REST API Erweiterungen
- **DELETE-Endpoints** für vollständige CRUD-Operationen:
  - DELETE `/api/v1/samples/:sample_id` - Probe löschen
  - DELETE `/api/v1/orders/:order_id` - Auftrag löschen
  - DELETE `/api/v1/results/:result_id` - Ergebnis löschen
  - Ressourcen-Existenzprüfung vor Löschung (404 bei nicht gefunden)
  - 204 No Content bei erfolgreicher Löschung
  - Audit-Logging mit Actor-Tracking
- **CORS-Support** für Frontend-Integration:
  - Access-Control-Allow-Origin: http://localhost:5173
  - Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS
  - Access-Control-Allow-Headers: Content-Type, X-API-Key, Authorization
  - OPTIONS Preflight-Handling
- **Verbesserte Fehlerbehandlung**:
  - Konsistente JSON-Fehlerantworten
  - HTTP-Statuscodes nach REST-Konventionen
  - Backend-Error-Message-Extraktion im Frontend

### Verbesserungen

- **Build-System**: CMake-Integration für jwt-cpp und OpenSSL
- **Dokumentation**:
  - HTTPS_QUICK_START.md - TLS-Setup-Anleitung
  - frontend/README.md - Frontend-Dokumentation
  - frontend/DEVELOPMENT.md - Entwickler-Guide
  - frontend/QUICK_START.md - Frontend-Schnellstart
- **Code-Qualität**:
  - TypeScript für Type-Safety im Frontend
  - ESLint-Konfiguration
  - Konsistente Error-Handling-Patterns
- **Sicherheit**:
  - JWT-Token-basierte Authentifizierung
  - TLS/HTTPS-Verschlüsselung
  - Passwort-Hashing (DJB2 mit Salt)
  - Input-Validierung in API und Frontend

### Breaking Changes

- **Authentifizierung**: API-Key wird durch JWT-Tokens ersetzt (mit Rückwärtskompatibilität)
- **Frontend**: Neue React-basierte UI anstelle CLI-only

### Technische Details

- **Frontend-Stack**: React 18, TypeScript, Vite, TailwindCSS, React Router, Axios
- **Backend-Dependencies**: jwt-cpp v0.7.0, OpenSSL 3.x
- **API-Version**: v1 (keine Breaking Changes in bestehenden Endpoints)
- **Browser-Kompatibilität**: Moderne Browser mit ES2020+ Support

## [0.2.0] - 2026-01-02

### Neue Features

#### 🆕 Auftragsverwaltung (Order-Modul)
- **Order-Datenmodell** mit Status-Workflow:
  - Status: REQUESTED → IN_PROGRESS → COMPLETED → VALIDATED → CANCELLED
  - Priorität: NORMAL, URGENT, EMERGENCY
  - Verknüpfung zu Proben via sampleId
- **CRUD-Operationen** für Orders in Database
- **CLI-Integration**: Menüpunkte 20-26 für Order-Verwaltung
- **Dateien**:
  - `include/core/Order.h` - Order-Datenmodell
  - `src/core/Order.cpp` - Implementierung
  - `test/unit/test_order.cpp` - Unit-Tests (8 Tests)

#### 🆕 Ergebniseingabe (TestResult-Modul)
- **TestResult-Datenmodell** mit Validierungs-Workflow:
  - Status: PENDING → REVIEWED → VALIDATED → REJECTED → AMENDED
  - Flags: NORMAL, ABNORMAL, CRITICAL, INCONCLUSIVE
  - Referenzbereiche (minValue, maxValue) mit automatischer Flag-Berechnung
- **CRUD-Operationen** für TestResults in Database
- **CLI-Integration**: Menüpunkte 30-36 für Ergebnis-Verwaltung
- **Dateien**:
  - `include/core/TestResult.h` - TestResult-Datenmodell
  - `src/core/TestResult.cpp` - Implementierung
  - `test/unit/test_testresult.cpp` - Unit-Tests (10 Tests)

#### 🆕 Gerätedatenschnittstelle (CSV-Ergebnisimport)
- **CsvResultImport-Klasse** für Laborgerätedaten:
  - Import von Analysegeräte-Ergebnissen im CSV-Format
  - Automatische Flag-Berechnung basierend auf Referenzbereichen
  - Verknüpfung mit bestehenden Orders
- **Format**: `order_id,parameter,value,unit,min_value,max_value`
- **Fehlertolerantes Parsing** mit detaillierter Statistik
- **Dateien**:
  - `include/utils/CsvResultImport.h` - Header
  - `src/utils/CsvResultImport.cpp` - Implementierung
  - `test/unit/test_csvresultimport.cpp` - Unit-Tests (5 Tests)

#### 🆕 Audit-Trail (rudimentär)
- **AuditEntry-Datenmodell** für lückenlose Protokollierung:
  - EntityType: SAMPLE, ORDER, RESULT, USER, SYSTEM
  - ActionType: CREATE, UPDATE, DELETE, VIEW, VALIDATE, LOGIN, LOGOUT
  - Zeitstempel, Benutzer, Details
- **Automatisches Logging** bei allen CRUD-Operationen
- **CLI-Integration**: Menüpunkte 50-51 für Audit-Anzeige
- **Dateien**:
  - `include/core/AuditEntry.h` - AuditEntry-Datenmodell
  - `src/core/AuditEntry.cpp` - Implementierung

#### 🆕 Benutzer-Authentifizierung
- **User-Datenmodell** mit Rollen-System:
  - Rollen: ADMIN, OPERATOR, VIEWER
  - Aktiv/Inaktiv-Status
  - Passwort-Hashing (DJB2 mit Salt)
- **Authentifizierung** mit Login/Logout
- **Berechtigungsprüfung** im CLI:
  - Admin: Vollzugriff inkl. Benutzerverwaltung
  - Operator: Erstellen, Bearbeiten, Löschen
  - Viewer: Nur Lesezugriff
- **CLI-Integration**: Menüpunkte 40-46 für Benutzerverwaltung
- **Dateien**:
  - `include/core/User.h` - User-Datenmodell
  - `src/core/User.cpp` - Implementierung

### Kritische Bugfixes

#### 🔴 HIGH - SQLite Foreign Key Enforcement
- **Problem**: SQLite Foreign Keys waren definiert aber nicht aktiviert
- **Lösung**: `PRAGMA foreign_keys = ON` nach Datenbankverbindung
- **Datei**: `src/db/Database.cpp`

### Verbesserungen

- **Test-Suite erweitert**: Von 18 auf 62 Tests
- **CLI um 26 neue Menüpunkte** erweitert
- **Datenbank-Schema** um 4 neue Tabellen erweitert (orders, test_results, audit_log, users)
- **Namespace-Struktur** beibehalten (opensylab::core, opensylab::db, opensylab::utils)

## [0.1.1] - 2025-11-25

### Kritische Bugfixes

#### 🔴 HIGH - Automatisierte Tests hinzugefügt
- **Problem**: Keine automatisierten Tests vorhanden; fehlender Regressionsschutz
- **Lösung**:
  - Einfaches Test-Framework ohne externe Abhängigkeiten implementiert
  - Unit-Tests für Sample-Klasse (6 Tests)
  - Unit-Tests für Database-Klasse (7 Tests)
  - Unit-Tests für CsvImport-Klasse (5 Tests)
  - Test-Runner mit farbiger Ausgabe
  - Gesamt: 18 automatisierte Tests
- **Dateien**:
  - `test/CMakeLists.txt` - Test-Konfiguration
  - `test/unit/test_runner.cpp` - Test-Framework
  - `test/unit/test_sample.cpp` - Sample-Tests
  - `test/unit/test_database.cpp` - Database-Tests
  - `test/unit/test_csvimport.cpp` - CSV-Import-Tests
  - `test_and_build.sh` - Build & Test-Skript

#### 🟡 MEDIUM - Eingabevalidierung in CLI
- **Problem**: CLI akzeptiert leere/whitespace-belegte IDs; Datenbank enthält unbrauchbare Datensätze
- **Lösung**:
  - `readValidatedInput()` Funktion mit Pflichtfeldprüfung
  - `trim()` Funktion entfernt führende/nachfolgende Whitespaces
  - `isValidId()` prüft auf gültige Zeichen (alphanumerisch, -, _)
  - `isEmpty()` erkennt leere/whitespace-Strings
  - Benutzerfreundliche Fehlermeldungen mit Wiederholungsschleife
- **Dateien**:
  - `include/utils/CliInterface.h` - Neue Validierungsmethoden
  - `src/utils/CliInterface.cpp` - Implementierung

#### 🟡 MEDIUM - CSV-Import Pflichtfeldprüfung
- **Problem**: CSV-Import akzeptiert Zeilen ohne Pflichtfelder; signalisiert Fehler nur über STDERR
- **Lösung**:
  - Pflichtfeldvalidierung für `sample_id` und `patient_id`
  - Whitespace-Prüfung mit `std::invalid_argument` Exception
  - Detaillierte Fehlerstatistik (✓ Erfolgreich / ✗ Fehler)
  - Klare Unterscheidung zwischen "keine Daten" und "Fehler aufgetreten"
  - Fehlerbehandlung mit Zeilennummern
- **Dateien**:
  - `src/utils/CsvImport.cpp` - Verbesserte Validierung

#### 🟡 MEDIUM - Database::getAllSamples Fehlerbehandlung
- **Problem**: Bei SQL-Fehlern wird leerer Vektor zurückgegeben; CLI meldet fälschlich "Keine Proben"
- **Lösung**:
  - `hasError()` Methode zur Fehlererkennung
  - `clearError()` Methode zum Zurücksetzen des Fehlerzustands
  - CLI prüft nun `hasError()` vor Anzeige
  - Unterscheidung zwischen leerem Ergebnis und Fehler
  - Exception-Handling beim Iterieren über Ergebnisse
- **Dateien**:
  - `include/db/Database.h` - Neue Fehlerbehandlungsmethoden
  - `src/db/Database.cpp` - Verbesserte getAllSamples()
  - `src/utils/CliInterface.cpp` - Fehlerprüfung in handleListSamples() und handleStatistics()

### Verbesserte Benutzerfreundlichkeit
- Konsistente Unicode-Symbole (✓ ✗ ℹ) für bessere Lesbarkeit
- Klare Fehler- und Erfolgsmeldungen in allen Modulen
- Verbesserte Ausgabe mit Formatierung

### Entwickler-Tools
- Neues Skript: `test_and_build.sh` - Kompiliert und testet in einem Schritt
- Aktualisiertes `build.sh` - Vereinfachter Build-Prozess

## [0.1.0] - 2025-11-24

### Initial Release
- Grundlegende Projektstruktur
- C++17-basierte Implementierung
- SQLite-Datenbank-Integration
- CLI-Interface
- CSV-Import-Funktion
- Probenverwaltung (CRUD)
- Modulare Architektur
- CMake Build-System
- Basis-Dokumentation
