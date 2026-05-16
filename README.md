# OpenSylab LIMS

**High-Performance, Clinical-Grade Laboratory Information Management System**

[![Version](https://img.shields.io/badge/version-0.8.2-blue)](CHANGELOG.md)
[![License](https://img.shields.io/badge/license-MIT-green)](#lizenz)
[![Tests](https://img.shields.io/badge/tests-181%20passing-brightgreen)](#tests)
[![C++](https://img.shields.io/badge/C%2B%2B-17-orange)](src/)
[![React](https://img.shields.io/badge/React-18-61dafb)](frontend/)
[![TypeScript](https://img.shields.io/badge/TypeScript-strict-blue)](frontend/src/)
[![Security](https://img.shields.io/badge/security-PBKDF2%20%7C%20JWT%20%7C%20TOTP-red)](#sicherheit)

OpenSylab ist eine moderne, native LIMS-Plattform für die medizinische Diagnostik. Entwickelt für maximale Zuverlässigkeit, Geschwindigkeit und kompromisslose Datensicherheit, bietet es eine schlanke Alternative zu überladenen Enterprise-Systemen.

---

## Warum OpenSylab?

OpenSylab bricht mit der Trägheit klassischer Laborsysteme durch radikales Engineering:

*   🚀 **Native Performance:** Ein C++17 Core ohne schwerfällige Frameworks ermöglicht Latenzen im Millisekundenbereich und minimalen Ressourcenverbrauch — ideal für High-Throughput-Umgebungen und Edge-Computing direkt am Analysegerät.
*   🛡️ **Security by Default:** Militärische Sicherheitsstandards sind fest integriert. PBKDF2-Hashing, JWT-Authentifizierung und MFA (TOTP) schützen Patientendaten ab dem ersten Byte.
*   ⚖️ **ISO 15189 Ready:** Ein unveränderlicher, revisionssicherer Audit-Trail auf Datenbankebene sorgt für lückenlose Nachvollziehbarkeit — Compliance ist kein Feature, sondern das Fundament.
*   ⚡ **Precision Engineering UI:** Ein "Neo-Clinical Industrial" Interface, das auf maximale Lesbarkeit und Fehlervermeidung in Stresssituationen optimiert ist. Keine Spielereien, nur produktiver Fokus.
*   🔌 **Seamless Interop:** Native, hocheffiziente Parser für HL7 v2.5.1 und FHIR R4 ermöglichen eine reibungslose Integration in bestehende Krankenhaus-Informationssysteme (KIS).

---

## Screenshots

### Dashboard
<!-- SCREENSHOT: Hauptansicht nach Login -->
<!-- Aufnahme: http://192.168.10.140:5173/ (als admin eingeloggt) -->
<!-- Zeigt: KPI-Kacheln (Proben, Aufträge, Ergebnisse, Kritisch), Aktivitäts-Charts, letzte Proben-Tabelle -->
![Dashboard](docs/screenshots/dashboard.png)
*Echtzeit-Übersicht: Proben, Aufträge, kritische Ergebnisse und Aktivitätsdiagramm*

### Proben-Verwaltung
<!-- SCREENSHOT: Seite /samples mit mindestens 5 Einträgen verschiedener Status -->
<!-- Aufnahme: Filter auf "Alle Status", Suche leer, erste Seite -->
<!-- Zeigt: Tabelle mit sample_id, patient_id, Status-Badge, Aktions-Buttons -->
![Samples](docs/screenshots/samples.png)
*Probenliste mit Status-Filter, Barcode-Scan und Soft-Delete (Status → ARCHIVED)*

### Auftrags-Verwaltung
<!-- SCREENSHOT: Seite /orders mit Aufträgen verschiedener Prioritäten -->
<!-- Aufnahme: Enthält mindestens je einen NORMAL, URGENT, EMERGENCY-Auftrag -->
<!-- Zeigt: Prioritäts-Badge-Farben (grau/orange/rot), Status-Workflow-Spalte -->
![Orders](docs/screenshots/orders.png)
*Aufträge nach Priorität (NORMAL / URGENT / EMERGENCY) und Status-Workflow*

### Ergebnisse & Auto-Flag
<!-- SCREENSHOT: Seite /results mit Ergebnissen verschiedener Flags -->
<!-- Aufnahme: Enthält NORMAL (grün), LOW (blau), HIGH (orange), CRITICAL (rot) Flags -->
<!-- Zeigt: Flag-Farb-Badges, Referenzbereich-Spalte, numerischer Messwert -->
![Results](docs/screenshots/results.png)
*Testergebnisse mit automatischer Flaggung: NORMAL / LOW / HIGH / CRITICAL*

### Ergebnis erfassen — Referenzbereich & Auto-Flag
<!-- SCREENSHOT: Modal "Neues Ergebnis" geöffnet, Wert außerhalb Referenzbereich eingegeben -->
<!-- Aufnahme: Wert = 15.2, Ref Min = 4.0, Ref Max = 11.0 → Flag zeigt automatisch "HIGH" -->
<!-- Zeigt: Formular mit Auto-Flag-Berechnung in Echtzeit -->
![Result Create](docs/screenshots/result_create.png)
*Ergebniseingabe mit automatischer Flag-Berechnung bei Abweichung vom Referenzbereich*

### Login & MFA
<!-- SCREENSHOT: Login-Seite, dann TOTP-Eingabe-Step -->
<!-- Aufnahme: 2 Screenshots nebeneinander oder als GIF: Credentials → TOTP-Code -->
<!-- Zeigt: Zweistufiger Login-Flow mit TOTP-Feld -->
![Login](docs/screenshots/login.png)
*JWT-Login mit optionalem TOTP-Zweifaktor (RFC 6238, Google Authenticator kompatibel)*

### Audit-Log
<!-- SCREENSHOT: Seite /audit-log mit mehreren Einträgen, Filter-Panel oben -->
<!-- Aufnahme: Mindestens CREATE, UPDATE, DELETE Aktionen sichtbar; Export-Button sichtbar -->
<!-- Zeigt: Tabelle mit Timestamp, Benutzer, Aktion, Entity-Typ, Details -->
![Audit](docs/screenshots/audit.png)
*Vollständiger ISO-15189-konformer Audit-Trail mit Filterung und CSV-Export*

### Import (CSV / HL7 / FHIR)
<!-- SCREENSHOT: Import-Seite, Tab "HL7 v2.5.1" aktiv, Datei geladen -->
<!-- Aufnahme: Zeige alle drei Tabs (CSV / HL7 / FHIR), HL7-Tab mit Datei ausgewählt -->
<!-- Zeigt: Tab-Leiste, Dateiname, Import-Button -->
![Import](docs/screenshots/import.png)
*Batch-Import für CSV-Proben, HL7 v2.5.1 (ORU^R01) und FHIR R4 Bundles*

### Benutzerverwaltung
<!-- SCREENSHOT: Seite /users (nur als ADMIN sichtbar) -->
<!-- Aufnahme: Tabelle mit mind. 3 Benutzern verschiedener Rollen (ADMIN, OPERATOR, VIEWER) -->
<!-- Zeigt: Rollen-Badges, Aktiv/Inaktiv-Status, Edit-Button -->
![Users](docs/screenshots/users.png)
*RBAC-Rollenverwaltung: ADMIN / OPERATOR / VIEWER / CUSTOM*

### Dark Mode 
<!-- SCREENSHOT: Dashboard im Dark Mode (sudo-Tastenkombination aktiviert) -->
<!-- Aufnahme: s-u-d-o eingeben außerhalb Inputs → Terminal-Industrial-Theme -->
<!-- Zeigt: Acid-Green (#CCFF00), Cyan (#00F0FF) Akzente auf dunklem Hintergrund -->
![Dark Mode](docs/screenshots/dark_mode.png)
*Terminal Industrial Dark Mode mit Neon-Akzenten — aktiviert via `sudo`-Tastenkombination*

---

## Features — v0.8.2

### Labordaten-Verwaltung
| Feature | Beschreibung |
|---------|-------------|
| **Probenverwaltung** | CRUD, Barcode-Scan (BarcodeDetector API), Status-Workflow: REGISTERED → IN_ANALYSIS → ANALYZED → VALIDATED → ARCHIVED |
| **Auftragsverwaltung** | Verknüpfung mit Proben, Prioritäten (NORMAL / URGENT / EMERGENCY), Status-Workflow, Transition-Validierung im Backend |
| **Ergebniseingabe** | Auto-Flag bei Eingabe: NORMAL / LOW / HIGH / **CRITICAL** (margin-basiert: 50 % des Referenzintervalls) — manuelle Überschreibung möglich |
| **Soft-Delete** | Proben → ARCHIVED, Aufträge → CANCELLED, Ergebnisse → REJECTED — Zeilen bleiben für Audit-Trail erhalten |
| **Referenzbereiche** | Pro Ergebnis `reference_low` / `reference_high`, Flag wird bei jedem Update neu berechnet |
| **Paginierung** | Server-seitige Paginierung auf allen Listen-Endpoints (limit / offset) |
| **Globale Suche** | Header-Suchleiste navigiert per `?q=` zu Samples oder Orders |

### Datenimport / -export
| Feature | Beschreibung |
|---------|-------------|
| **Batch-CSV-Import (Proben)** | RFC 4180, BOM-tolerant, 5-MB-Limit, Fehler-Tracking pro Zeile |
| **Batch-CSV-Import (Ergebnisse)** | Multiline-Felder mit korrektem Escape-State-Machine |
| **HL7 v2.5.1** | ORU^R01 Import + Export via HTTP-API (`/api/v1/hl7/import`) mit vollständigem Feld-Escaping |
| **FHIR R4** | Bundle-Import (Patient, Specimen, ServiceRequest, Observation) + Export via HTTP-API |
| **Audit-Log-Export** | CSV-Export (ADMIN only) mit konfigurierbarer Retention-Policy |
| **Statistiken** | Dashboard-Kacheln, Statusverteilung, kritische Ergebnisse (Echtzeit, Backend-aggregiert) |

### Sicherheit
| Feature | Beschreibung |
|---------|-------------|
| **JWT-Authentifizierung** | HMAC-SHA256, Ablaufzeit konfigurierbar via `OPENSYLAB_JWT_SECRET` |
| **PBKDF2-Passwort-Hashing** | 210.000 Iterationen, Random-Salt, Salt-Leer-Guard, konstanter Zeitvergleich (OWASP 2023) |
| **RBAC** | 4 Rollen: ADMIN / OPERATOR / VIEWER / CUSTOM — auf allen Schreib-Endpoints erzwungen, Auth-Check vor JSON-Parse |
| **Rate-Limiting** | 10 Versuche / 60 s pro IP auf `/api/v1/auth/login`, query-string-resistent |
| **MFA (TOTP)** | RFC 6238 HMAC-SHA1, ±1 Zeitfenster (90 s), Google Authenticator kompatibel |
| **LDAP** | Optionale LDAP-Authentifizierung mit lokalem Shadow-Account und Rollen-Mapping |
| **Letzter-Admin-Schutz** | `updateUser`, `deleteUser`, `assignUserRole` blockieren Demotierung des letzten Admins — transaktional, kein TOCTOU |
| **Self-Delete-Guard** | Admin kann sich nicht selbst deaktivieren |
| **Audit bei allen Writes** | Jedes CREATE / UPDATE / DELETE erzeugt AuditEntry mit `user_id`, `action`, `entity`, `timestamp` |
| **Fehler-Sanitisierung** | HTTP-Antworten enthalten keine SQLite-Internals |
| **TLS/HTTPS** | `--tls-cert` / `--tls-key` Flags; `--force-https` verhindert HTTP-Betrieb in Prod |

### Frontend / UX
| Feature | Beschreibung |
|---------|-------------|
| **React 18 + TypeScript strict** | Keine `any`-Typen im Produktionscode, 0 TypeScript-Fehler |
| **RBAC im UI** | `canWrite`-Guards auf allen Create/Edit/Delete-Buttons; Sidebar-Links rollenabhängig |
| **Dark Mode** | Terminal-Industrial-Ästhetik via `sudo`-Tastenkombination (Easter Egg) |
| **Responsive Tabellen** | Sekundäre Spalten auf Tablet ausgeblendet (`md:hidden`) |
| **useEntityList-Hook** | Universeller paginierter Listen-Hook mit Abbruch-Mechanik, Race-Condition-sicher |
| **MFA-Login-Flow** | Zweistufiger Login: Credentials → TOTP-Code |
| **Sichere Logout-Navigation** | Sidebar-Logout navigiert sofort zu `/login` |
| **Health-Endpoint** | `GET /api/v1/health` — unauthentifiziert, liefert `{"status":"ok","version":"0.8.2"}` |
| **Version-SSOT** | `CMakeLists.txt` → `include/version.h` (C++); `package.json` → `VITE_APP_VERSION` (Frontend) |

### Qualität
| Metrik | Wert |
|--------|------|
| **Unit-Tests** | 181 passing |
| **TypeScript** | strict mode, 0 Errors |
| **npm audit** | 0 Vulnerabilities |
| **Bug-Hunts** | 12 Iterationen (diese Session), 3 aufeinanderfolgende saubere Runs — 21 Dateien, 30+ Bugs behoben. v0.8.2: 4 regressions fixed |
| **Gesamt Bug-Hunts** | 68 Iterationen total, 90+ Bugs behoben |

---

## Technologie-Stack

### Backend
- **Sprache**: C++17
- **Datenbank**: SQLite3 (embedded, kein externer Server nötig)
- **Auth**: PBKDF2-HMAC-SHA256 (Passwörter) · HMAC-SHA256 JWT · HMAC-SHA1 TOTP
- **Kryptographie**: OpenSSL
- **Build**: CMake 3.15+

### Frontend
- **Framework**: React 18 + TypeScript (strict mode)
- **Build**: Vite 7
- **Routing**: React Router v6
- **HTTP**: Axios mit JWT-Interceptor + Token-Expiry-Guard
- **Styling**: Tailwind CSS 3
- **Charts**: Recharts

---

## Schnellstart

### Voraussetzungen
- CMake ≥ 3.15
- GCC / Clang mit C++17-Support
- OpenSSL
- Node.js ≥ 18
- SQLite3 (Entwicklungsbibliotheken)

### Build & Start

```bash
# 1. Repository klonen
git clone https://github.com/AurevionSec/openSylab.git
cd openSylab

# 2. Backend bauen
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)

# 3. Server starten (Port 9080, DB wird automatisch angelegt)
./build/bin/OpenSylab --api --api-port 9080 --db opensylab.db

# 4. Frontend bauen & starten
cd frontend
npm install
echo "VITE_API_URL=http://localhost:9080/api/v1" > .env.production
npm run build
npx serve dist --listen 5173 --single
```

Browser öffnen: **http://localhost:5173**  
Standard-Login: `admin` / `admin` → **sofort Passwort ändern!**

### Docker

```bash
docker compose up -d
```

Detaillierte Anleitung: [docs/DOCKER.md](docs/DOCKER.md)

---

## Konfiguration

| Umgebungsvariable | Standard | Beschreibung |
|-------------------|---------|-------------|
| `OPENSYLAB_JWT_SECRET` | dev-secret | JWT-Signaturschlüssel **(in Prod: zufällig generieren!)** |
| `OPENSYLAB_CORS_ORIGIN` | `http://localhost:5173` | Erlaubte Frontend-Origin |
| `OPENSYLAB_DB_PATH` | `opensylab.db` | Datenbankpfad |
| `OPENSYLAB_TLS_CERT` | — | Pfad zum TLS-Zertifikat (PEM) |
| `OPENSYLAB_TLS_KEY` | — | Pfad zum TLS-Schlüssel (PEM) |

```bash
# Produktionsstart mit TLS
OPENSYLAB_JWT_SECRET="$(openssl rand -hex 32)" \
OPENSYLAB_CORS_ORIGIN="https://lims.meinlabor.de" \
./build/bin/OpenSylab \
  --api --api-port 9443 \
  --tls-cert /etc/ssl/lims.crt \
  --tls-key  /etc/ssl/lims.key \
  --force-https \
  --db /var/lib/opensylab/lims.db
```

---

## Architektur

```
┌─────────────────────────────────────────────────┐
│  Frontend (React 18 / TypeScript)               │
│  http://localhost:5173                          │
└──────────────────┬──────────────────────────────┘
                   │ HTTPS / JWT
┌──────────────────▼──────────────────────────────┐
│  REST API  (C++17)                              │
│  Port 9080/9443  ·  /api/v1/*                  │
│                                                 │
│  Layer 4: ApiServer   (HTTP-Routing, TLS, CORS) │
│  Layer 3: JwtAuth     (Token-Validierung, RBAC) │
│  Layer 2: Utils       (CSV, HL7, FHIR, CLI)     │
│  Layer 1: Database    (SQLite3-Persistenz)      │
│  Layer 0: Core        (Domain-Entities)         │
└──────────────────┬──────────────────────────────┘
                   │
┌──────────────────▼──────────────────────────────┐
│  SQLite3  (embedded)  ·  opensylab.db           │
└─────────────────────────────────────────────────┘
```

**Schichtenregel:** Layer N darf nur Layer N-1 importieren — keine Circular Dependencies.

---

## API-Übersicht

| Methode | Endpunkt | Beschreibung | Auth |
|---------|----------|-------------|------|
| `GET` | `/api/v1/health` | Health-Check + Version | — |
| `POST` | `/api/v1/auth/login` | JWT-Login (+ MFA) — rate-limited | — |
| `GET` | `/api/v1/samples` | Probenliste (`?q=`, `?status=`) | JWT |
| `POST` | `/api/v1/samples` | Probe anlegen | OPERATOR+ |
| `PUT` | `/api/v1/samples/:id` | Probe aktualisieren (Transition validiert) | OPERATOR+ |
| `GET` | `/api/v1/orders` | Auftragsliste | JWT |
| `POST` | `/api/v1/orders` | Auftrag anlegen | OPERATOR+ |
| `GET` | `/api/v1/results` | Ergebnisliste (`?flag=`, `?status=`) | JWT |
| `POST` | `/api/v1/results` | Ergebnis eingeben | OPERATOR+ |
| `GET` | `/api/v1/audit` | Audit-Log (gefiltert, exportierbar) | ADMIN |
| `GET` | `/api/v1/users` | Benutzerliste | ADMIN |
| `POST` | `/api/v1/hl7/import` | HL7 v2.5.1 ORU^R01 Import | OPERATOR+ |
| `POST` | `/api/v1/fhir/import` | FHIR R4 Bundle Import | OPERATOR+ |
| `GET` | `/api/v1/stats` | Dashboard-Statistiken (Backend-aggregiert) | JWT |

Vollständige API-Dokumentation: [docs/API.md](docs/API.md)

---

## Tests

```bash
# Backend-Tests ausführen
cmake --build build && ./build/bin/opensylab_tests

# Frontend-Typprüfung
cd frontend && npx tsc --noEmit

# CI-äquivalent (mit Timeout)
timeout 60 ./build/bin/opensylab_tests
```

**75 Unit-Tests passing** — Backend (C++): Datenbank, Domain-Entities, API-Router, CSV-Import, HL7, FHIR, Statistiken, Utils.

Weitere Details: [docs/TESTING.md](docs/TESTING.md)

---

## Versionierung

Die Versionsnummer hat **eine** kanonische Quelle pro Layer:
- **C++**: `CMakeLists.txt` → `project(VERSION x.y.z)` → generiert `include/version.h` im Build-Tree
- **Frontend**: `frontend/package.json` → `"version"` → `import.meta.env.VITE_APP_VERSION`

Siehe [docs/VERSIONING.md](docs/VERSIONING.md) für den Release-Prozess.

---

## Mitwirken

1. Fork & Branch: `git checkout -b feat/mein-feature`
2. Implementieren + Tests schreiben
3. `cmake --build build && timeout 60 ./build/bin/opensylab_tests` — alle Tests grün
4. `cd frontend && npx tsc --noEmit` — 0 TypeScript-Fehler
5. Pull Request öffnen

---

## Lizenz

MIT — siehe [LICENSE](LICENSE)

---

## Changelog

Vollständige Versionshistorie: [CHANGELOG.md](CHANGELOG.md)

**Aktuelle Version: [0.8.2](CHANGELOG.md#082---2026-05-14)** — Test-Regression-Fix: alle 181 Tests grün (isValidStatusString-Refactor).
