# OpenSylab — Roadmap & TODO

**Aktuelle Version:** v0.7.0 (2026-05-11)
**Nächste Version:** v0.8.0
**Branch:** main

---

## ✅ v0.7.0 — Abgeschlossen (2026-05-11)

Vollständige Änderungsliste: [CHANGELOG.md](CHANGELOG.md#070---2026-05-11)

Highlights: JWT-Auth · PBKDF2 · RBAC · MFA/TOTP · LDAP · Soft-Delete ·
Auto-Flag · Batch-CSV-Import · HL7 · FHIR · Audit-Trail · 181 Unit-Tests ·
60+ Bugfixes aus intensivem Bughunt · Single Source of Truth für Version

---

## 🎯 v0.8.0 — Analyse & Planung

### P0 — Sicherheit (Blocker für Production)

- [x] **Kein Standard-JWT-Secret in VCS** — `docker-compose.yml` enthält
      `change-this-secret-key-in-production` als Klartextsecret.
      Fix: Secret aus Datei/Vault lesen, Startup-Fehler wenn dev-secret in Prod.
- [x] **Erzwungener Passwort-Wechsel** bei erstem Login mit `admin/admin`
      (kein first-run guard vorhanden — Prod-Deployments laufen mit Standardcreds)
- [x] **HTTPS erzwingen** — `--force-https` Flag für Prod, HTTP→HTTPS Redirect
      (`--tls` Flag existiert in main.cpp, aber kein Enforcement)
- [x] **Rate Limiting** auf `/api/v1/auth/login` — aktuell 0 Schutz gegen
      Credential Stuffing (single-threaded, kein Login-Counter)
- [x] **API-Key-Fallback entfernen oder härten** — X-API-Key-Auth bypassed RBAC
      vollständig (wird als OPERATOR behandelt, keine Rollen-Prüfung möglich)

### P1 — Fehlende Kernfeatures

- [x] **Health-Endpoint** `GET /api/v1/health` — fehlt komplett;
      Docker-Healthcheck deaktiviert, Reverse-Proxy-Probes schlagen fehl
- [x] **HL7/FHIR API-Endpoints** — `Hl7Exchange` + `FhirExchange` sind
      vollständig implementiert (Hl7.cpp, Fhir.cpp) aber in ApiServer.cpp
      nicht verdrahtet. Zero HTTP-Routes für `/api/v1/hl7/*` und `/api/v1/fhir/*`.
- [x] **Audit-Log-Export UI** — `Database::exportAuditLogToCsv()` existiert im
      Backend, aber kein HTTP-Endpoint und kein Export-Button auf der Audit-Seite
- [x] **Status-Transition-Validierung im Backend** — nur im Frontend per
      `SAMPLE_TRANSITIONS`/`ORDER_TRANSITIONS` erzwungen; das Backend akzeptiert
      jeden Status-String auf PUT (ISO 15189 Compliance-Lücke)
- [ ] **Konfigurationsdatei** `opensylab.conf` — aktuell alles via CLI-Flags /
      Env-Vars; kein standardisierter Konfig-Pfad für Prod-Deployments
- [ ] **Frontend Unit-Tests** — 0 automatisierte Frontend-Tests (Vitest + RTL).
      Docs: "geplant für v0.8.0"
- [ ] **OpenAPI / Swagger** — kein maschinenlesbarer API-Contract, 30+ Endpoints
      ohne Dokumentation
- [ ] **Datenbankmigrationen** — kein versioniertes Migration-System;
      Schema-Upgrade von v0.7 → v0.8 = manuell oder DB-Reset

### P2 — UI/UX-Verbesserungen

- [x] **Create-Button auf Results-Seite** ohne `canWrite`-Guard —
      VIEWER sieht den Button (schlägt beim Submit fehl, aber verwirrend)
- [x] **Breadcrumb-Bug** — `/audit-log` wird als "Dashboard" angezeigt
      (`routeNames` hat `/audit` statt `/audit-log` in Header.tsx)
- [x] **Suche: inkonsistente Prefix-Logik** — `O` (ohne Bindestrich) → Orders,
      `R-` (mit Bindestrich) → Results; `RES-001` landet silently bei Samples
- [x] **Import-Seite: nur Samples** — UI ruft nur `createSample()` auf;
      Ergebnis-Import (CsvResultImport.cpp), HL7, FHIR sind nicht erreichbar
- [x] **Dashboard Statistik-Fallback** — bei > 100 Einträgen werden Kacheln
      und Diagramme unvollständig (client-seitige Aggregation mit limit:100)
- [x] **Order-ID in Ergebnissen** — `resultToJson()` sendet numerische PK,
      nicht den lesbaren String wie `O-2024-001`; Tabelle zeigt Zahlen statt IDs
- [x] **`updated_at` bei Proben** — `sampleToJson()` hat kein `updated_at`-Feld;
      Frontend zeigt immer `registration_date` als "zuletzt geändert"
- [x] **Live-Zähler in Sidebar-Badges** — `badge: '24'` in Sidebar ist
      hardcoded und wird nie gerendert (toter Code)
- [x] **Passwort-Stärke-Indikator** auf der Profil-Seite
- [x] **TESTING.md stale** — zeigt "62 Tests" statt aktuell 181

### P3 — Infrastruktur / Operations

- [x] **CI/CD Pipeline** — kein `.github/workflows/`; kein automatischer
      Build/Test/Push auf PR oder merge
- [x] **Backend-Healthcheck in docker-compose.yml** — aktuell auskommentiert;
      Frontend-Container startet ggf. vor dem Backend
- [x] **CORS-Duplikation** — `getenv("OPENSYLAB_CORS_ORIGIN")` wird in
      `handleClientTls()` und `handleClientPlain()` separat gelesen (2 Stellen)
- [ ] **Single-threaded Server** — `serveLoop()` verarbeitet Verbindungen
      sequenziell; ein langsamer Client blockiert alle anderen
- [ ] **Geheimer Schlüssel-Rotation** — kein dokumentierter Prozess zum
      Rotieren von JWT-Secret ohne Server-Neustart

---

## Gesamtübersicht v0.8 nach Aufwand

| Kategorie | Items | Aufwand (geschätzt) |
|-----------|-------|---------------------|
| P0 Sicherheit | 5 | ~2 Wochen |
| P1 Kernfeatures | 8 | ~4 Wochen |
| P2 UI/UX | 10 | ~2 Wochen |
| P3 Infrastruktur | 5 | ~1 Woche |
| **Gesamt** | **28** | **~9 Wochen** |

**Empfohlener MVP-Scope für v0.8.0** (fokussiert auf Production-Readiness):
P0 komplett + Health-Endpoint + HL7/FHIR-Endpoints + Breadcrumb-Fix + TESTING.md

---

## Bekannte technische Schulden (kein Release-Blocker)

- `token_expiry`-Key wird in `auth.ts` geschrieben aber in `api.ts` als `_expiry`
  gelesen — wenn der Key fehlt, stiller Logout bei jedem Request
- Test-Runner nutzt eigenes Macro-Framework statt Catch2/GoogleTest → kein
  Standard-CI-Output ohne Custom-Skripte
- Keine E2E/Integration-Tests (Playwright/Cypress)

---

*Zuletzt aktualisiert: 2026-05-11 — Analyse nach v0.7.0-Release*
