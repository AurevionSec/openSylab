# OpenSylab_

> **Analoge Zuverlässigkeit. Digitale Präzision.**
> Das Open-Source LIMS für Labore, die auf Daten-Souveränität und Performance setzen.

[![Version](https://img.shields.io/badge/Version-0.6.0-success.svg?style=for-the-badge)]()
[![Core](https://img.shields.io/badge/Backend-C%2B%2B17-00599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)]()
[![Interface](https://img.shields.io/badge/Frontend-React_18-20232A.svg?style=for-the-badge&logo=react&logoColor=61DAFB)]()
[![License](https://img.shields.io/badge/license-OpenSylab-black?style=for-the-badge)](LICENSE.txt)

---

## /// SYSTEM ARCHITECTURE & VISION

Proprietäre LIMS sind Blackboxen. Sie diktieren Prozesse und halten Daten als Geiseln. **OpenSylab** ist die systemische Antwort auf dieses Problem. Es ist ein kostenfreies, quelloffenes Laborinformationssystem, entwickelt für die klinische Diagnostik.

Wir kombinieren ein kompromisslos schnelles C++17 Backend mit einer modernen, TypeScript-basierten React-SPA. Keine Cloud-Pflicht. Volle Kontrolle vor Ort.

---

## ⚡ CORE CAPABILITIES (v0.6.0)

Wir bauen Infrastruktur, keine Spielzeuge. Version 0.6.0 liefert ein vollständiges LIMS-Frontend und RBAC.

### 1. Workflow Engine (Proben & Aufträge)
- **Barcode-Ready:** Eindeutige Proben-IDs mit vollständigem Status-Tracking (Erfasst bis Archiviert).
- **Real-Time Validation:** Automatische Plausibilitätsprüfung von Testergebnissen gegen Referenzbereiche inkl. Flagging (NORMAL, LOW, HIGH, CRITICAL).
- **Device Connectivity:** Direkter CSV-Batch-Import und HL7-Parser für Laborgeräte.

### 2. Tactical Dashboard & UI
- **Single Page Application:** Reaktives Interface via React 18 & TailwindCSS 4.
- **Multi-Entity Stats:** Server-seitig aggregierte Echtzeit-Daten und Status-Breakdowns im Dashboard.
- **Data Filtering:** Dual-Filtering für Aufträge (Status + Priority) und Ergebnisse (Status + Flag).
- **Dark Theme:** Terminal Industrial Ästhetik mit Neon-Akzenten für 24/7 Lab Operations.

### 3. Governance & Compliance (ISO 15189)
- **Audit Trail:** Unveränderliche Protokollierung aller Systemänderungen (Wer, Was, Wann) mit dediziertem Admin-Viewer.
- **Role-Based Access Control (RBAC):** Granulare Rechteverteilung (ADMIN, OPERATOR, VIEWER, CUSTOM) auf Frontend- und API-Ebene.
- **Security:** JWT-Authentifizierung (HS256) und TLS 1.2+ via OpenSSL.

---

## 🚀 DEPLOYMENT (Zero-Friction)

Vergiss manuelle Kompilierung. Das System ist Docker-ready.

```bash
# 1. Repository klonen
git clone https://github.com/AurevionSec/openSylab.git
cd openSylab

# 2. Container zünden
docker compose up -d

# 3. Access
# Web-UI: http://localhost
# API:    http://localhost:8080/api/v1

# 4. First Login
# Username: admin
# Password: admin (⚠️ SOFORT ÄNDERN!)
```

**Für bare-metal Installationen via CMake:** siehe [`docs/INSTALL.md`](docs/INSTALL.md).

---

## 💀 SECURITY BRIEFING (READ BEFORE PROD)

Version 0.6.0 ist produktionsnah, erfordert aber systemische Disziplin.
Folgende Vektoren müssen für externe Deployments geschlossen werden:

- ⚠️ **Default Credentials:** Der Account `admin/admin` MUSS sofort geändert werden (Profile → Change Password).
- ⚠️ **JWT Secret:** Das JWT Secret in `docker-compose.yml` (Zeile 22) ist ein Platzhalter. Generiere ein sicheres Secret:
  ```bash
  openssl rand -base64 48
  ```
  Trage es in `docker-compose.yml` unter `OPENSYLAB_JWT_SECRET` ein.
- ⚠️ **Hashing:** Aktuell läuft DJB2 (nur für Development). Migration auf bcrypt/argon2 erfolgt in v0.7.
- ⚠️ **TLS/HTTPS:** Für Production-Deployments TLS-Zertifikate einbinden (siehe [`docs/DOCKER.md`](docs/DOCKER.md)).

**Für Production-Deployments:** siehe [`docs/DOCKER.md`](docs/DOCKER.md) für vollständige Sicherheitskonfiguration.

---

## 🛠 TECH STACK

- **Engine:** C++17, CMake 3.15+, SQLite 3.x.
- **API:** Custom HTTP-Server (30+ Endpoints, JSON-basiert).
- **Frontend:** React 18, TypeScript, Vite 5, Axios.
- **Quality Assurance:** 62 passed Unit-Tests (Backend).
- **Deployment:** Docker Compose, Multi-Stage Builds.

---

## 🗺 ROADMAP (v0.7.0+)

Wir iterieren schnell und gnadenlos.

- **Security Lock-down:** HTTPS-Enforcement, bcrypt Hashing.
- **API-Docs:** OpenAPI 3.0 / Swagger UI.
- **Scale:** PostgreSQL Support und Multi-Tenant-Fähigkeit.

Details im [`ROADMAP.MD`](ROADMAP.MD).

---

## ⚖️ LIZENZ & COMMUNITY

Open Source. Freie Verwendung für nicht-kommerzielle Zwecke.
Datenkommerz und Vendor-Lock-in bleiben draußen. Kommerzielle Nutzung erfordert Genehmigung.

**Details:** [`LICENSE.txt`](LICENSE.txt).

Bug gefunden? Feature Request? Pull Requests und fachlicher Input von Laborfachkräften sind willkommen.

---

**Entwickelt mit klinischer Präzision.**
**Deployed mit industrieller Effizienz.**
**OpenSylab – Labor. Code. Souveränität.**
