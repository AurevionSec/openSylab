# OpenSylab - Entwicklungs-Roadmap

Diese Roadmap beschreibt die schrittweise Entwicklung von OpenSylab von der aktuellen CLI-basierten Version bis zum vollwertigen LIMS mit Web-Interface, API und erweiterten Features.

## Versionierungs-Philosophie

OpenSylab folgt dem Prinzip **"Stabilität vor Features"**. Jede Version wird erst freigegeben, wenn sie gründlich getestet ist und zuverlässig funktioniert. Neue Funktionen werden nur dann integriert, wenn die bereits vorhandenen stabil laufen.

## Version 0.1.1 (Aktuell) ✓ ABGESCHLOSSEN

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

### Technologie-Stack

- C++17
- CMake 3.15+
- SQLite 3.x
- Eigenes Test-Framework (keine externen Abhängigkeiten)

### Architektur

```text
src/
├── core/     - Geschäftslogik (Sample-Klasse)
├── db/       - Datenbank-Zugriffsschicht
├── utils/    - CLI + CSV-Import
└── main.cpp  - Hauptprogramm
```

---

## Version 0.2 - Erweiterte CLI & PostgreSQL

**Status:** Geplant
**Geschätzter Zeitrahmen:** Q1 2026

### Ziele

Stabilisierung der v0.1-Basis und Hinzufügen erster Multi-User-Fähigkeiten durch PostgreSQL-Support.

### Geplante Features

#### Kern-Features

- **PostgreSQL-Unterstützung**: Alternative zu SQLite für größere Installationen
  - Multi-User-fähige Datenbank
  - Transaktionsmanagement
  - Datenbank-Abstraktionsschicht für beide Backends

- **Erweiterte CLI-Funktionen**:
  - Erweiterte Suchfunktionen (Filter nach Datum, Status, Patient)
  - Batch-Operationen (mehrere Proben auf einmal aktualisieren)
  - Export-Funktionen (CSV, JSON)
  - Konfigurationsdateien für Datenbankverbindung

- **Benutzer-Authentifizierung** (Grundversion):
  - Login-System mit Passwort-Hashing (bcrypt/Argon2)
  - Einfache Rollenverwaltung (Admin, User)
  - Session-Management

- **Audit-Trail** (Grundversion):
  - Protokollierung wichtiger Aktionen
  - Wer hat wann was geändert
  - Unveränderbare Log-Einträge

### Technische Verbesserungen

- Konfigurationsmanagement (JSON/YAML-Dateien)
- Verbessertes Logging-System
- Datenbank-Migrationen (Schema-Versionierung)
- Performance-Optimierungen

### Tests

- Erweiterte Unit-Tests (Ziel: 30+ Tests)
- Erste Integrationstests
- PostgreSQL-spezifische Tests

---

## Version 0.5 - REST-API & Web-Interface (Alpha)

**Status:** Konzeptphase
**Geschätzter Zeitrahmen:** Q3 2026

### Ziele

Einführung einer REST-API und eines Web-Frontends als Alternative zur CLI. Dies ist der Übergang von einer Desktop-Anwendung zu einem netzwerkfähigen System.

### Geplante Features

#### API-Schicht

- **REST-API** (C++ Backend):
  - Framework: drogon oder CppRestSDK
  - Endpoints für alle CRUD-Operationen
  - JSON-basierte Kommunikation
  - HTTPS/TLS-Verschlüsselung
  - JWT-Authentifizierung
  - OpenAPI/Swagger-Dokumentation

#### Web-Frontend (Alpha)

- **Technologie**: React oder Angular
- **Funktionen**:
  - Dashboard mit Übersicht
  - Probenverwaltung (analog zur CLI)
  - Suchfunktionen
  - CSV-Import über Upload
  - Einfache Statistiken
  - Responsive Design (Desktop + Tablet)

#### Erweiterte Features

- **Verbesserte Benutzer-/Rollenverwaltung**:
  - Feingranulare Berechtigungen (RBAC)
  - Mehrere Benutzerrollen (Admin, Laborant, Qualitätsbeauftragter)
  - Audit-Trail für Zugriffe

- **Erweiterte Statistiken**:
  - Grafische Darstellung von Kennzahlen
  - Zeitreihen-Analysen
  - Export von Reports

### Architektur

```text
┌─────────────┐
│ Web-Browser │
└──────┬──────┘
       │ HTTPS/REST
┌──────▼──────┐
│  API Server │ (C++)
│  (drogon)   │
└──────┬──────┘
       │
┌──────▼──────┐     ┌─────────┐
│  PostgreSQL │◄────┤   CLI   │
└─────────────┘     └─────────┘
```

### Tests

- API-Tests (REST-Endpoints)
- Frontend-Unit-Tests (Jest/Jasmine)
- End-to-End-Tests (Selenium/Cypress)

---

## Version 1.0 - Produktionsreifes LIMS

**Status:** Vision
**Geschätzter Zeitrahmen:** Q2 2027

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

1. **Höchste Priorität**: Stabilität und Zuverlässigkeit der Kern-Features
2. **Hohe Priorität**: PostgreSQL-Support und Basis-API (v0.2-0.5)
3. **Mittlere Priorität**: Web-Interface und erweiterte Features (v0.5-1.0)
4. **Niedrige Priorität**: KI und Multi-Site (v2.0+)

---

## Meilensteine

| Version | Status | Fortschritt | Geschätztes Release |
|---------|--------|-------------|---------------------|
| v0.1.1  | ✅ Abgeschlossen | 100% | 2025-11-25 |
| v0.2    | 📋 Geplant | 0% | Q1 2026 |
| v0.5    | 💡 Konzept | 0% | Q3 2026 |
| v1.0    | 💡 Vision | 0% | Q2 2027 |
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

**Letzte Aktualisierung:** 2025-11-25
**Version des Dokuments:** 1.1
