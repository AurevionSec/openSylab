# OpenSylab — Roadmap & TODO

**Aktuelle Version:** v0.8.2 (2026-05-14)
**Nächste Version:** v0.9.0
**Branch:** main

---

## ✅ v0.7.0 — Abgeschlossen (2026-05-11)

Vollständige Änderungsliste: [CHANGELOG.md](CHANGELOG.md#070---2026-05-11)

Highlights: JWT-Auth · PBKDF2 · RBAC · MFA/TOTP · LDAP · Soft-Delete ·
Auto-Flag · Batch-CSV-Import · HL7 · FHIR · Audit-Trail · 181 Unit-Tests ·
60+ Bugfixes aus intensivem Bughunt · Single Source of Truth für Version

---

## ✅ v0.8.x — Abgeschlossen (2026-05-14)

Vollständige Änderungsliste: [CHANGELOG.md](CHANGELOG.md)

Highlights: Rate Limiting · Erzwungener Passwort-Wechsel · HTTPS erzwingen · Health-Endpoint · HL7/FHIR API-Endpoints · Audit-Log-Export · Status-Transition-Validierung im Backend · CI/CD Pipeline · 43 Bug-Hunt-Iterationen · Security-Hardening · 181 Unit-Tests grün

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
- [x] **Socket-Timeouts implementieren** — Fehlendes `SO_RCVTIMEO` / `SO_SNDTIMEO`
      im `ApiServer` macht das System anfällig für Slowloris-DoS-Angriffe.

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
- [ ] **Multi-threaded Server / Concurrency** — Aktuelle `serveLoop()` ist
      sequenziell/blockierend; ein langsamer/böswilliger Client blockiert
      die gesamte API für alle anderen Benutzer. Umstellung auf Thread-Pool
      oder Thread-per-Connection erforderlich.
- [ ] **Geheimer Schlüssel-Rotation** — kein dokumentierter Prozess zum
      Rotieren von JWT-Secret ohne Server-Neustart

---

## Gesamtübersicht v0.8 nach Aufwand

| Kategorie | Items | Aufwand (geschätzt) |
|-----------|-------|---------------------|
| P0 Sicherheit | 6 | ~2.5 Wochen |
| P1 Kernfeatures | 8 | ~4 Wochen |
| P2 UI/UX | 10 | ~2 Wochen |
| P3 Infrastruktur | 5 | ~1.5 Wochen |
| **Gesamt** | **29** | **~10 Wochen** |

**Empfohlener MVP-Scope für v0.8.0** (fokussiert auf Production-Readiness):
P0 komplett + Health-Endpoint + HL7/FHIR-Endpoints + Breadcrumb-Fix + TESTING.md

---

## Bekannte technische Schulden (kein Release-Blocker)

- `token_expiry`-Key wird in `auth.ts` geschrieben aber in `api.ts` als `_expiry`
  gelesen — wenn der Key fehlt, stiller Logout bei jedem Request
- Test-Runner nutzt eigenes Macro-Framework statt Catch2/GoogleTest → kein
  Standard-CI-Output ohne Custom-Skripte
- Keine E2E/Integration-Tests (Playwright/Cypress)
- **JSON-Parser Einschränkungen** — Handgeschriebener Parser in `ApiServer.cpp`
  unterstützt keine Arrays oder verschachtelten Objekte; erschwert API-Ausbau.

---

## Strategische Analyse-Findings (2026-05-16)

### Kritische Bugs (neu identifiziert)

- [x] **Status-Enum-Mismatch Frontend/Backend** — `constants.ts` definiert
      `RESULT_TRANSITIONS` mit `REVIEWED`/`AMENDED`; `ApiServer.cpp` nutzt
      `ENTERED`/`REPEATED`. Frontend zeigt Transitions an, die das Backend ablehnt.
      Stiller ISO 15189-Compliance-Bug. Datei: `frontend/src/utils/constants.ts`,
      `src/api/ApiServer.cpp` (`kResultTrans`).
- [x] **SQLite WAL-Mode nicht aktiviert** — Ohne `PRAGMA journal_mode=WAL` blockiert
      jeder Write alle parallelen Reads. Bei Batch-CSV-Import ist der Server für die
      Dauer des Imports für alle anderen Nutzer blockiert.
      Fix: `PRAGMA journal_mode=WAL;` in `Database::initializeSchema()`.
- [ ] **HTTP-Header-Truncation bei >8192 Bytes** — `handleClientPlain` /
      `handleClientTls` lesen im ersten `recv`/`SSL_read` exakt 8192 Bytes.
      Sehr lange Authorization-Header (z.B. große JWTs, viele Cookies) werden stumm
      abgeschnitten — Auth schlägt dann ohne erklärbaren Fehler fehl.
      Fix: Header-Akkumulation analog zur Body-Akkumulation via Content-Length.
- [x] **Keine Security-Header in HTTP-Responses** — Kein `Strict-Transport-Security`,
      kein `X-Content-Type-Options`, kein `X-Frame-Options` in den API-Antworten.
      Relevant besonders für den Plain-HTTP-Pfad und Browser-Clients.
- [ ] **Kein JWT-Token-Blacklisting nach Logout** — Tokens bleiben bis zu ihrer
      Ablaufzeit (60 min) gültig. Bei kompromittierten Credentials ist der
      Angreifer für dieses Fenster autorisiert. Fix: Redis-basierte Blacklist oder
      kurzlebige Tokens + Refresh-Token-Rotation.
- [ ] **Kein TOTP Base32-Enrollment-Flow dokumentiert** — TOTP-Secrets werden
      offenbar als Rohstring gespeichert. Kompatibilität mit Standard-Authenticator-
      Apps (Google Authenticator, Authy) via QR-Code-Enrollment prüfen.

### Architektur-Schulden (Mittel- bis Langfrist)

- [ ] **`ApiServer.cpp` aufteilen (God-File, ~3400 Zeilen)** — Alle ~30 Route-Handler,
      JSON-Parser, URL-Decoder, Rate-Limiter, CORS-Logik in einer einzigen Datei.
      Refactoring: Je eine Handler-Klasse pro Ressource (SampleHandler, OrderHandler,
      ResultHandler, UserHandler, AuditHandler, StatsHandler, HL7Handler, FhirHandler).
      Aufwand: ~3–4 Wochen.
- [ ] **JSON-Parser ersetzen: nlohmann/json via FetchContent** — Eigenentwicklung
      unterstützt keine Arrays oder verschachtelten Objekte. nlohmann/json ist
      header-only, zero transitive Dependencies, exzellenter Security-Track-Record.
      FetchContent-Infrastruktur existiert bereits (jwt-cpp). Aufwand: ~1 Woche.
- [ ] **Layer-Verletzung: API importiert DB direkt** — `include/api/ApiServer.h`
      importiert `db/Database.h` direkt; verletzt die dokumentierte 5-Layer-Regel
      (Layer 4 darf nicht Layer 1 importieren). Langfristig ein Repository-/Service-
      Pattern einführen als Mediator.
- [ ] **PostgreSQL-Backend-Option** — SQLite ist für Single-Lab-Edition korrekt;
      für Multi-Site / Multi-Tenant (Roadmap v1.1) braucht es eine DB-Abstraktions-
      schicht mit PostgreSQL-Support. Roadmap v0.9 sieht das vor.

### v0.9.0 — Architektur-Modernisierung (Prio-Vorschlag)

- [ ] **Thread-Pool / Thread-per-Connection** (→ bestehend in P3, hier priorisiert)
- [ ] **Socket-Timeouts** `SO_RCVTIMEO` / `SO_SNDTIMEO` (→ bestehend in P0)
- [ ] **nlohmann/json einbinden** (→ neu, s.o.)
- [ ] **SQLite WAL-Mode** (→ neu, s.o.)
- [ ] **Status-Enum-Mismatch** Frontend/Backend (→ neu, s.o.)
- [ ] **Hash-Chain Audit-Trail** — Kryptografisch verkettete Audit-Einträge: jeder
      Hash enthält den Hash des vorherigen Eintrags. Manipulierter Eintrag bricht
      die gesamte Chain — mathematisch nachweisbar. Optional: RFC 3161-qualifizierter
      Zeitstempel (QTSP nach eIDAS) pro Hash für forensische Verwertbarkeit.
      Aufwand: ~3–4 Wochen (Implementation + Verifikations-Tool + Dokumentation).
- [ ] **OpenAPI / Swagger** (→ bestehend in P1)
- [ ] **Datenbankmigrationen** (→ bestehend in P1)
- [ ] **PostgreSQL-Backend-Option** (→ neu, s.o.)
- [ ] **Strukturierte JSON-Logs** — Aktuell kein dediziertes Logging-System;
      `std::cout`/`std::cerr` in Library-Code verletzt die CLAUDE.md-Regel.

---

## Strategische Roadmap-Erweiterungen

### STR-1: IQ/OQ/PQ-Validierungspaket (ISO 15189)

Kein Code — Dokumentation. Installationsqualifizierung (IQ), Betriebsqualifizierung
(OQ), Leistungsqualifizierung (PQ) nach DIN EN ISO 15189 pro OpenSylab-Version.
Mit automatisierten Testskripten, die den OQ-Prozess reproduzierbar machen.

- Zielgruppe: DAkkS-akkreditierte Labore (~2.000 in Deutschland), ÖKAS (Österreich),
  SAS (Schweiz)
- Marktrelevanz: Validierungsdokumentation kostet bei externen Beratern 5.000–20.000 €;
  OpenSylab kann das mit Eigenkenntnis für 3.000–10.000 € anbieten
- Moat: Nur durch direkte Zusammenarbeit mit akkreditierten Laboren und regulatorisches
  Domain-Know-how erstellbar; ein Fork repliziert das nicht automatisch
- Aufwand: ~6–10 Wochen initial, ~2–3 Wochen pro Major-Version-Update

### STR-2: ASTM/LIS02-A2 Geräte-Konnektivität

Native C++-Implementierung des ASTM E1394/LIS02-A2-Protokolls (RS-232/TCP-basierter
Standard für Gerät-LIS-Kommunikation). Ergebnisse kommen direkt vom Analysegerät —
keine manuelle Eingabe. Erste Zielgeräte: Sysmex XN-Serie (Hämatologie) oder
Roche cobas c (klinische Chemie).

- Zielgruppe: Labore mit automatisierten Analysegeräten — die Kunden mit echtem Budget
- Moat: Gerätehersteller-spezifische Edge Cases sind nur durch physischen Gerätezugang
  lernbar; wer das dokumentiert hat, ist Monate voraus
- Aufwand: ~4–6 Wochen pro Geräteklasse (mit Testzugang oder Simulator)

### STR-3: Signiertes Docker-Image + SBOM ("OpenSylab Verified")

Cosign/Sigstore-signierte Docker-Images, Software Bill of Materials (SPDX/CycloneDX),
öffentliches CVE-Scan-Dashboard via Trivy in CI/CD. Verifizierungs-Workflow prüft
beim Start die Integrität des Images.

- Zielgruppe: Krankenhauslabore in KRITIS-regulierten Infrastrukturen, IT-Security-
  Verantwortliche mit Anforderungen an Software-Herkunftsnachweise
- Aufwand: ~2–3 Wochen für Setup, danach automatisiert in CI/CD

### STR-4: OpenSylab Academy — Zertifizierung für Laborinformatiker

Kostenpflichtiges Online-Schulungsprogramm (Teachable/Podia):
- Modul 1: OpenSylab Administration (~200–300 €)
- Modul 2: ISO 15189 Compliance mit OpenSylab (~300–500 €)
- Modul 3: Geräteintegration & API (~200–300 €)
Mit anerkanntem Zertifikat "Certified OpenSylab Administrator".

- Zielgruppe: Laborinformatiker, IT-Admins in Laboren, LIMS-Consultants
- Strategischer Nebeneffekt: Jeder Absolvent = potenzieller Consulting-Kunde;
  Academy ist gleichzeitig Vertriebs-Funnel
- Aufwand: ~4–6 Wochen für erste Kursversion

---

## Monetarisierungsreihenfolge (Empfehlung)

**Phase 1 (v0.9–v1.0): Support & Consulting — sofort umsetzbar**
- Deployment & Hardening: 1.500–5.000 € pro Engagement
- ISO 15189-Validierungsdokumentation für Labore: 3.000–10.000 €
- Schulungen: 500–1.500 € pro Session
- Voraussetzung: keine — sofort Revenue-generierend

**Phase 2 (v1.0–v1.2): Compliance-as-a-Service + Open Core**
- Produktifiziertes IQ/OQ/PQ-Paket: 5.000–15.000 € Erstvalidierung + 1.500–3.000 €/Jahr
- Open Core: proprietäre Enterprise-Features (Geräte-Konnektoren STR-2,
  Hash-Chain-Audit mit eIDAS-Zeitstempel, Multi-Site-Management)
- MIT-Kern bleibt öffentlich; proprietäre Features strukturell isoliert
- Voraussetzung: STR-1 (Validierungspaket) muss fertig sein

**Phase 3 (v1.3+): Managed SaaS — erst nach DSGVO-Klärung**
- Preismodell: Starter 99 €/Mo · Professional 299 €/Mo · Enterprise 999 €/Mo
- Voraussetzung: AVV mit jedem Kunden, BSI-C5-zertifiziertes RZ, ISO 27001,
  MDR-Klärung (Medizinprodukt?), Multi-Tenant-Architektur (Roadmap v1.1)
- Zeitrahmen: frühestens 12–18 Monate Vorlauf vor erstem Umsatz

**Phase 4 (langfristig): OEM-Deals, Marketplace**
- OEM mit mittelgroßen IVD-Herstellern (Sysmex, Mindray) über Referenz-Implementierungen
- Plugin-Marketplace erst sinnvoll ab ~1.000 GitHub Stars + aktiver Community
- Plugin-Architektur technisch jetzt vorbereiten (Extension-Points für Geräte-Konnektoren)

---

*Zuletzt aktualisiert: 2026-05-16 — Strategische Analyse*
