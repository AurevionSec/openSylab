# OpenSylab - Entwicklungs-Roadmap

Diese Roadmap beschreibt die schrittweise Entwicklung von OpenSylab von der aktuellen CLI-basierten Version bis zum vollwertigen LIMS mit Web-Interface, API und erweiterten Features.

## Versionierungs-Philosophie

OpenSylab folgt dem Prinzip **"Stabilität vor Features"**. Jede Version wird erst freigegeben, wenn sie gründlich getestet ist und zuverlässig funktioniert. Neue Funktionen werden nur dann integriert, wenn die bereits vorhandenen stabil laufen.

## Version 0.1.1 ✓ ABGESCHLOSSEN

**Status:** Stabile CLI-basierte Grundversion
**Release-Datum:** 2025-11-25

### Implementierte Features

- ✅ **CLI-Interface**: Menügeführte Benutzeroberfläche mit interaktiver Steuerung
- ✅ **Probenverwaltung**: Vollständige CRUD-Operationen (Create, Read, Update, Delete)
- ✅ **SQLite-Datenbank**: Zuverlässige lokale Datenpersistenz
- ✅ **CSV-Import**: Batch-Import mit Pflichtfeldvalidierung
- ✅ **Eingabevalidierung**: Pflichtfeldprüfung mit Wiederholungsschleife
- ✅ **Fehlerbehandlung**: Klare Fehlerpropagierung von DB bis CLI
- ✅ **Status-Tracking**: 5 Probenstatus (Erfasst, In Analyse, Analysiert, Validiert, Archiviert)
- ✅ **Test-Framework**: 18 Unit-Tests (6 Sample, 7 Database, 5 CSV-Import)
- ✅ **Dokumentation**: Vollständige Dokumentation (README, CHANGELOG, TESTING)

---

## Version 0.2.0 ✓ ABGESCHLOSSEN

**Status:** Erweiterte CLI mit LIMS-Kernfunktionen
**Release-Datum:** 2026-01-02

### Implementierte Features

- ✅ **Auftragsverwaltung**: Order-Modul mit Status und Prioritäten
  - Status: REQUESTED, IN_PROGRESS, COMPLETED, VALIDATED, CANCELLED
  - Prioritäten: NORMAL, URGENT, EMERGENCY
  - Verknüpfung mit Proben via sampleId

- ✅ **Ergebniseingabe**: TestResult-Modul mit Plausibilitätsprüfung
  - Status: PENDING, ENTERED, VALIDATED, REJECTED, REPEATED
  - Flags: NORMAL, LOW, HIGH, CRITICAL, UNDEFINED
  - Automatische Flag-Berechnung anhand Referenzbereichen

- ✅ **Gerätedatenschnittstelle**:
  - CSV-Import für Ergebnisdaten
  - HL7-Parser für Laborgeräte
  - Batch-Import mit Validierung

- ✅ **Audit-Trail**: Vollständige Protokollierung
  - Wer, Was, Wann für jede Aktion
  - Unveränderliche Audit-Einträge
  - Aktionstypen: CREATE, UPDATE, DELETE, LOGIN, etc.

- ✅ **Benutzerauthentifizierung**:
  - Login/Logout-Funktionalität
  - Rollen: USER, ADMIN, LAB_TECH, VALIDATOR
  - Passwort-Hashing mit SHA-256
  - Admin-Benutzerverwaltung

- ✅ **Test-Framework**: 62 Unit-Tests
  - Sample-, Order-, TestResult-, Database-, CSV-Import-Tests

### Technologie-Stack

- C++17
- CMake 3.15+
- SQLite 3.x
- Eigenes Test-Framework (keine externen Abhängigkeiten)

### Architektur

```text
src/
├── core/     - Geschäftslogik (Sample, Order, TestResult, AuditEntry, User)
├── db/       - Datenbank-Zugriffsschicht
├── utils/    - CLI + CSV-Import + HL7-Parser
└── main.cpp  - Hauptprogramm
```

---

## Version 0.5.0 ✓ ABGESCHLOSSEN

**Status:** REST-API & React Web-Interface (Alpha)
**Release-Datum:** 2026-02-01

### Implementierte Features

#### API-Schicht

- ✅ **REST-API** (C++ Backend):
  - Custom HTTP-Server mit JSON-Support
  - Endpoints für alle CRUD-Operationen (GET, POST, PUT)
  - Core Endpoints: /api/v1/samples, /api/v1/orders, /api/v1/results
  - HTTPS/TLS-Verschlüsselung mit OpenSSL 3.x
  - JWT-Authentifizierung mit jwt-cpp v0.7.0
  - CORS-Unterstützung für Frontend
  - Konsistente Fehlerbehandlung

#### Web-Frontend (Alpha)

- ✅ **React 18 + TypeScript**:
  - Vite 5 Build-Tool mit HMR
  - TailwindCSS 3 für responsive UI
  - React Router 6 für SPA-Navigation
  - Axios HTTP-Client mit JWT-Interceptors

- ✅ **Funktionen**:
  - Dashboard mit Übersicht
  - Probenverwaltung (Samples): Liste, Filtering, Detail-Ansicht
  - Auftragsverwaltung (Orders): Liste, Dual-Filtering (Status + Priority)
  - Ergebnisverwaltung (Results): Liste, Dual-Filtering (Status + Flag)
  - Protected Routes mit JWT-Auth
  - Real-time Formularvalidierung
  - Responsive Design (Desktop + Tablet)

#### Authentifizierung & Sicherheit

- ✅ **JWT-basierte Authentifizierung**:
  - Token-basierte Auth (JWT mit HS256)
  - Login-Endpoint mit Username/Password
  - 60-Minuten Token-Gültigkeit
  - Automatische Token-Validierung
  - Rückwärtskompatibilität mit API-Key

- ✅ **TLS/HTTPS**:
  - OpenSSL-Integration
  - TLS 1.2+ mit modernen Cipher Suites
  - Optionaler HTTPS-Modus
  - Zertifikatsverwaltung

### Architektur

```text
┌─────────────┐
│ Web-Browser │ (React + TypeScript)
└──────┬──────┘
       │ HTTPS/REST + JWT
┌──────▼──────┐
│  API Server │ (C++ Custom HTTP Server)
│  + JWT Auth │
└──────┬──────┘
       │
┌──────▼──────┐     ┌─────────┐
│   SQLite    │◄────┤   CLI   │
└─────────────┘     └─────────┘
```

### Tests

- ✅ 62 Backend Unit-Tests
- API-Tests für alle Endpoints (manuell)
- Frontend-Integration (lokal getestet)

---

## Version 0.6.0 ✓ ABGESCHLOSSEN (Aktuell)

**Status:** User Management & Enhanced UI
**Release-Datum:** 2026-02-03

### Implementierte Features

#### User Management System

- ✅ **Admin-only Benutzerverwaltung**:
  - CRUD Interface für User Management
  - Role-Based Access Control (RBAC)
  - Rollen: ADMIN, OPERATOR, VIEWER, CUSTOM
  - Benutzer aktivieren/deaktivieren
  - Vollständige Benutzerdaten (Name, E-Mail, Rolle)

- ✅ **User Profile Management**:
  - Benutzerprofil-Seite für alle User
  - Passwortänderung mit Validierung
  - Anzeige von User-Informationen
  - Last Login Tracking

#### Audit Log & Compliance

- ✅ **Audit Log Viewer**:
  - Admin-only Audit Trail Anzeige
  - Filterung nach User, Action, Entity, Zeitraum
  - Compliance-Tracking für ISO 15189
  - Unveränderliche Audit-Einträge
  - Detaillierte Aktionsprotokolle (CREATE, UPDATE, DELETE, LOGIN, LOGOUT)

#### Enhanced Dashboard

- ✅ **Multi-Entity Statistiken**:
  - Samples Status-Breakdown mit Zählungen
  - Orders Status-Breakdown mit Zählungen
  - Results Status-Breakdown mit Zählungen
  - Real-time Daten vom Backend
  - Server-seitige Aggregation

#### API-Erweiterungen

- ✅ **8 neue API-Endpoints**:
  - GET/POST /api/v1/users - User Management (Admin only)
  - PUT /api/v1/users/:id - Update User (Admin only)
  - DELETE /api/v1/users/:id - Delete User (Admin only)
  - GET /api/v1/users/me - Get Current User Profile
  - PUT /api/v1/users/me/password - Change Password
  - GET /api/v1/audit - Audit Log (Admin only)
  - GET /api/v1/stats - Dashboard Statistics

- ✅ **Role-Based Access Control auf API-Ebene**:
  - JWT-Token mit Role-Information
  - Endpoint-basierte Zugriffssteuerung
  - 403 Forbidden für unberechtigte Zugriffe

#### Frontend-Erweiterungen

- ✅ **3 neue UI-Pages**:
  - Users Page: Admin-only User Management mit CRUD
  - Audit Log Page: Admin-only Compliance-Viewer
  - Profile Page: User Profile mit Passwortänderung

- ✅ **UI/UX Improvements**:
  - Role-basierte Navigation (Sidebar-Filterung)
  - Modal-basierte Create/Edit Forms
  - Color-coded Status Badges
  - Enhanced Protected Routes mit requiredRole
  - TypeScript Type Safety für alle Entities

### Architektur-Verbesserungen

```text
┌─────────────────────────┐
│ React Frontend (TS)     │
│ - Dashboard (Enhanced)  │
│ - Samples/Orders/Results│
│ - Users (Admin)         │
│ - Audit Log (Admin)     │
│ - Profile               │
└──────────┬──────────────┘
           │ HTTPS/REST + JWT (with Roles)
┌──────────▼──────────────┐
│  API Server (C++)       │
│  - 30+ Endpoints        │
│  - RBAC Middleware      │
│  - Audit Logging        │
└──────────┬──────────────┘
           │
┌──────────▼──────────────┐
│   SQLite Database       │
│   - Users Table         │
│   - Audit_Entries Table │
│   - Stats Aggregation   │
└─────────────────────────┘
```

### Statistiken

- **22 neue Dateien** im Frontend
- **6,436 Insertions** (Code-Zeilen)
- **38 Deletions**
- **8 neue API-Endpoints**
- **3 neue UI-Pages**
- Version Bump: 0.2.0 → 0.6.0

### Bekannte Einschränkungen (v0.6.0)

⚠️ **NICHT für Produktion bereit** - Folgende Punkte müssen in v0.7.0 adressiert werden:

- **Password Hashing**: Aktuell DJB2 (nur Development!) - muss auf bcrypt/argon2 umgestellt werden
- **JWT Secret**: Hardcoded - muss externalisiert werden (Environment Variable)
- **Default Credentials**: admin/admin MÜSSEN geändert werden
- **HTTPS**: Optional - sollte für Produktion erzwungen werden

### Tests

- ✅ 62 Backend Unit-Tests (bestehend)
- API-Tests für neue Endpoints (manuell getestet)
- Frontend-Integration (lokal getestet)
- Login/Auth Flow vollständig getestet

---

## Version 0.7.0 - Production Security & Docker

**Status:** In Planung
**Geschätzter Zeitrahmen:** Q2 2026 (4-6 Wochen)

### Ziele

Produktionsreife Sicherheit, Docker-Containerisierung und Vervollständigung der CRUD-Operationen im Frontend. Diese Version fokussiert sich auf die Beseitigung aller bekannten Sicherheitslücken aus v0.6.0.

### Geplante Features (Priorität P0/P1)

#### Sicherheits-Verbesserungen (P0 - KRITISCH)

- **🔒 Produktionsreifes Password Hashing**:
  - Ersetzen von DJB2 durch bcrypt oder argon2
  - Password Strength Validation
  - Migration bestehender Passwort-Hashes
  - Password Strength Indicator in UI

- **🔒 Secrets Management**:
  - JWT-Secret aus Environment Variable
  - Konfigurationsdatei (opensylab.conf)
  - Dokumentation für Secret Rotation
  - --config CLI-Flag für Konfigurationsdatei

- **🔒 HTTPS Enforcement**:
  - Erzwungenes HTTPS in Production Mode
  - HTTP → HTTPS Redirect
  - Let's Encrypt Integration Guide
  - Zertifikats-Setup Dokumentation

#### Frontend CRUD Completion (P1)

- **Samples Management**:
  - Create Sample Modal/Form
  - Edit Sample Modal/Form
  - Delete Confirmation Dialog
  - Barcode Scanning Integration

- **Orders Management**:
  - Create Order Form
  - Edit Order Form
  - Link Orders to Samples
  - Status Workflow Transitions

- **Results Management**:
  - Result Entry Form
  - Validation Workflow
  - Plausibility Checking UI
  - Reference Range Validation

#### API-Erweiterungen (P1)

- **DELETE Endpoints**:
  - DELETE /api/v1/samples/:id (Soft-Delete)
  - DELETE /api/v1/orders/:id (Soft-Delete)
  - DELETE /api/v1/results/:id (Soft-Delete)
  - Audit Log für alle Deletes

#### Docker-Containerisierung (P1)

- **Production Docker Setup**:
  - Multi-stage Dockerfile für Backend
  - Dockerfile für Frontend (nginx)
  - Docker Compose für Production
  - Environment Variable Configuration
  - Volume Mounts für Datenpersistenz
  - Health Check Endpoints

#### Dokumentation (P1)

- **API-Dokumentation**:
  - OpenAPI 3.0 Specification
  - Dokumentation aller 30+ Endpoints
  - Request/Response Schemas
  - Swagger UI at /api/docs

- **Production Deployment Guide**:
  - Server Requirements (CPU, RAM, Disk)
  - Installation Steps (Ubuntu/Debian)
  - HTTPS Certificate Setup
  - Database Backup Procedures
  - Monitoring & Logging Setup
  - Security Hardening Checklist

#### Testing (P1)

- **Integration Tests**:
  - User Management CRUD Tests
  - Audit Log Filtering Tests
  - Statistics Endpoints Tests
  - RBAC Tests
  - Password Change Flow Tests

- **Frontend Tests**:
  - Jest + React Testing Library Setup
  - Authentication Context Tests
  - Protected Routes Tests
  - Component Tests (Users, Audit Log, Profile)

### Geschätzter Aufwand

- Sicherheit (P0): 5-7 Tage
- Frontend CRUD (P1): 10-12 Tage
- Docker (P1): 3-4 Tage
- Dokumentation (P1): 5-6 Tage
- Testing (P1): 5-6 Tage

**Gesamt:** 4-6 Wochen (bei Vollzeit-Entwicklung)

### Definition of Done

- ✓ Alle P0-Security-Tasks abgeschlossen
- ✓ bcrypt/argon2 implementiert und getestet
- ✓ JWT-Secret externalisiert
- ✓ HTTPS erzwungen in Production Mode
- ✓ Frontend CRUD vollständig für Samples, Orders, Results
- ✓ Docker Compose Setup funktioniert
- ✓ OpenAPI Dokumentation erstellt
- ✓ Production Deployment Guide geschrieben
- ✓ Integration Tests bestehen
- ✓ Frontend Tests bestehen

---

## Version 0.8.0 - PostgreSQL & Advanced Features

**Status:** Geplant
**Geschätzter Zeitrahmen:** Q3 2026

### Geplante Features (P2)

- **PostgreSQL Support**:
  - libpqxx Integration
  - Database Adapter Pattern
  - Connection Pooling
  - Migration System

- **Advanced Frontend Features**:
  - Data Visualization (Charts.js/Recharts)
  - CSV Import UI
  - Advanced Filtering
  - Export Functionality

- **Security Enhancements**:
  - Two-Factor Authentication (TOTP)
  - LDAP/Active Directory Integration
  - Rate Limiting

- **Reporting**:
  - PDF Report Generation
  - Excel/CSV Export
  - Custom Report Templates

---

## Version 1.0 - Produktionsreifes LIMS

**Status:** Vision
**Geschätzter Zeitrahmen:** Q4 2026 - Q1 2027

### Ziele

Erste produktionsreife Version mit vollständigem Feature-Set für kleine bis mittlere Labore.

### Geplante Features

#### Kern-LIMS-Funktionen

- **Vollständiger Workflow**:
  - Auftrags-/Testverwaltung
  - Arbeitslisten für Laboranten
  - Priorisierung (Notfallproben)
  - Mehrstufige Workflows

- **Befunderstellung**:
  - Template-basierte Befunde
  - PDF-Generierung
  - Elektronische Signatur
  - Referenzbereiche und Normwerte

- **Qualitätskontrolle**:
  - QC-Proben-Verwaltung
  - Levey-Jennings-Diagramme
  - Automatische Alarmierung bei Abweichungen
  - Kalibrier-Management

- **Geräteschnittstellen** (Grundversion):
  - CSV/XML-Import von Analysegeräten
  - Konfigurierbare Datei-Importer
  - Automatische Ergebniszuordnung

#### Erweiterte Features

- **Vollständiger Audit-Trail**:
  - Lückenlose Protokollierung
  - Fälschungssicher (21 CFR Part 11-konform)
  - Audit-Reports und Export

- **Erweiterte Authentifizierung**:
  - Zwei-Faktor-Authentifizierung (2FA)
  - LDAP/Active Directory-Anbindung
  - Single Sign-On (SSO)

- **Internationalisierung**:
  - Mehrsprachigkeit (DE, EN)
  - Lokalisierung (Datums-/Zahlformate)
  - Übersetzbare UI-Texte

- **Backup & Recovery**:
  - Automatische Backups
  - Wiederherstellungsfunktionen
  - Datenbank-Replikation

### Compliance & Regulierung

- ISO 15189-Vorbereitung (Dokumentation)
- DSGVO-Konformität
- Datenschutz-Folgenabschätzung
- Audit-fähiges QM-System

### Tests

- Umfassende Unit-Tests (100+ Tests)
- Integrationstests
- Performance-Tests
- Sicherheits-Audits
- Benutzerakzeptanz-Tests in Pilotlaboren

---

## Version 1.x - Stabilisierung & Erweiterungen

**Status:** Langfristige Planung
**Zeitrahmen:** 2027-2028

### Geplante Verbesserungen

- **v1.1**: Erweiterte Geräteschnittstellen (HL7, ASTM)
- **v1.2**: FHIR-Integration für Krankenhausinformationssysteme
- **v1.3**: Erweiterte Statistik- und BI-Funktionen
- **v1.4**: Mobile App (Android/iOS) für Laboranten
- **v1.5**: Inventar-/Reagenzienverwaltung

---

## Version 2.0 - Enterprise LIMS

**Status:** Vision
**Zeitrahmen:** 2028+

### Langfristige Ziele

- **Multi-Site-Betrieb**:
  - Mehrere Laborstandorte
  - Zentralisierte Datenverwaltung
  - Standortübergreifende Probenverfolgung

- **Hochverfügbarkeit**:
  - Cluster-Betrieb
  - Load Balancing
  - Failover-Mechanismen

- **KI-Integration**:
  - Automatische Ergebnisvalidierung (ML)
  - Prädiktive Analysen
  - Anomalieerkennung

- **Erweiterte Interoperabilität**:
  - Vollständige HL7/FHIR-Unterstützung
  - EDI-Schnittstellen
  - Abrechnungssysteme

- **Biobanking-Modul**:
  - Probensammlungen
  - Einlagerungs-Management
  - Forschungsdaten-Export

---

## Entwicklungs-Prinzipien

### Qualität über Geschwindigkeit

- Jede Version wird gründlich getestet
- Keine Features ohne Tests
- Code Reviews für alle Änderungen
- Regelmäßige Sicherheits-Audits

### Community-Driven

- Offener Entwicklungsprozess
- Feedback von Laboren fließt in Planung ein
- Transparente Entscheidungsfindung
- Kollaborative Entwicklung

### Rückwärtskompatibilität

- Datenbank-Migrationen zwischen Versionen
- API-Versionierung
- Dokumentierte Breaking Changes
- Upgrade-Pfade für alle Versionen

---

## Priorisierung

Die Roadmap ist flexibel und wird basierend auf Community-Feedback angepasst. Aktuelle Prioritäten:

1. **Höchste Priorität (P0)**: Produktionsreife Sicherheit (v0.7.0)
2. **Hohe Priorität (P1)**: Frontend CRUD Completion, Docker, API-Dokumentation (v0.7.0)
3. **Mittlere Priorität (P2)**: PostgreSQL, Advanced Features, Reporting (v0.8.0)
4. **Niedrige Priorität (P3)**: KI, Multi-Site, Enterprise Features (v2.0+)

---

## Meilensteine

| Version | Status | Fortschritt | Release-Datum |
|---------|--------|-------------|---------------|
| v0.1.1  | ✅ Abgeschlossen | 100% | 2025-11-25 |
| v0.2.0  | ✅ Abgeschlossen | 100% | 2026-01-02 |
| v0.5.0  | ✅ Abgeschlossen | 100% | 2026-02-01 |
| v0.6.0  | ✅ Abgeschlossen | 100% | 2026-02-03 |
| v0.7.0  | 🛠️ In Planung | 0% | Q2 2026 (4-6 Wochen) |
| v0.8.0  | 💡 Geplant | 0% | Q3 2026 |
| v1.0    | 💡 Vision | 0% | Q4 2026 - Q1 2027 |
| v2.0    | 💭 Langfristig | 0% | 2028+ |

---

## Feedback & Beiträge

Diese Roadmap ist ein lebendiges Dokument. Wir freuen uns über:

- **Feature-Requests**: Was braucht Ihr Labor?
- **Priorisierungs-Input**: Welche Features sind am wichtigsten?
- **Technische Vorschläge**: Bessere Lösungsansätze?
- **Community-Diskussionen**: GitHub Discussions

**Kontakt:**

- GitHub Issues: Feature-Requests und Bugs
- GitHub Discussions: Roadmap-Diskussionen
- Community-Meetings: Monatliche virtuelle Treffen

---

**Letzte Aktualisierung:** 2026-02-03
**Version des Dokuments:** 2.0 (v0.6.0 Release Update)
**Aktueller Release:** v0.6.0
**Nächster Release:** v0.7.0 (Production Security & Docker)
