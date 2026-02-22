# 🚀 OpenSylab Roadmap

OpenSylab entwickelt sich von einer leistungsfähigen
Open-Source-LIMS-Lösung hin zu einer professionellen, cloud-fähigen
Plattform mit klarer Trennung zwischen Self-Host-Edition und zukünftiger
Cloud-Edition.

Diese Roadmap beschreibt die geplanten Entwicklungsschritte von v0.8 bis
v1.5.

------------------------------------------------------------------------

# v0.8 -- Security & Engineering Stabilization

**Ziel:** Technische Vertrauensbasis schaffen

## Security Improvements

-   Entfernung aller Default-Secrets
-   Erzwungene Secret-Generierung beim ersten Start
-   Passwort-Hashing mit Argon2
-   Session-Hardening
-   Basis Rate-Limiting

## Engineering & Quality

-   CI/CD Pipeline
-   Unit-Test Coverage Reporting
-   Static Code Analysis (Backend & Frontend)
-   Dependency Security Scanning
-   Automatisch generierte OpenAPI-Spezifikation

## Documentation

-   Architekturdiagramm
-   Security Hardening Guide
-   Deployment Guide für Self-Hosting

------------------------------------------------------------------------

# v0.9 -- Architektur-Modernisierung

**Ziel:** Codebasis für Skalierung und Cloud-Readiness vorbereiten

## Datenbank

-   Offizielle PostgreSQL-Unterstützung
-   Datenbank-Migrationsframework
-   Saubere DB-Abstraktionsschicht

## Logging & Audit

-   Manipulationssichere Audit-Logs (Hash-Chain)
-   Strukturierte JSON-Logs

## Konfiguration

-   Environment-basierte Konfiguration
-   Vorbereitung für externes Secret-Management (Vault/KMS)

## API

-   Versionierte REST-API
-   Erste SDK-Unterstützung (Python)

------------------------------------------------------------------------

# v1.0 -- Stable Open-Source Edition

**Ziel:** Produktionsreife Self-Host-Version (Research Use)

## Sicherheit

-   Erweiterte RBAC
-   Optionale Multi-Factor-Authentication
-   Verbessertes Session-Management

## Reporting

-   Audit-Report-Export (PDF)
-   Benutzer- und Probenexport

## Integration

-   Stabilisierung der HL7-Unterstützung
-   Verbesserter CSV-Import
-   Vollständige API-Dokumentation

## Betrieb

-   Dokumentierte Backup-Strategie
-   Getestete Restore-Anleitung

------------------------------------------------------------------------

# v1.1 -- Multi-Tenant Grundlagen

**Ziel:** Mandantenfähigkeit vorbereiten

## Architektur

-   Tenant-ID auf Datenebene
-   Option für isolierte Datenbanken pro Tenant
-   Tenant-spezifische Konfiguration

## Sicherheit

-   Vorbereitung für Tenant-spezifische Encryption Keys
-   Storage-Abstraktion (S3-kompatibel)

------------------------------------------------------------------------

# v1.2 -- Compliance & Datenschutz

**Ziel:** Vorbereitung für regulatorische Nutzung im deutschen Markt

## Datenschutz

-   Datenexport gemäß DSGVO (Art. 15)
-   Löschkonzept
-   Konfigurierbare Logging-Retention

## Sicherheit

-   TLS-Zwang
-   Login-Versuchsüberwachung
-   Optionale IP-Whitelist

## Dokumentation

-   Technische & organisatorische Maßnahmen (TOM) Vorlage
-   Bedrohungsmodell
-   Risikoanalyse

------------------------------------------------------------------------

# v1.3 -- Cloud Edition Technical Foundation

**Ziel:** Technische Basis für zukünftige Cloud-Version

## Tenant Lifecycle

-   Tenant-Erstellung & Verwaltung
-   Tenant-spezifische Limits

## Monitoring

-   Health Dashboard
-   System-Metriken

## Backup

-   Automatisierte Backups
-   Dokumentierte Wiederherstellung

## Enterprise Security

-   OIDC / SSO Integration
-   Verpflichtende MFA für Admins

------------------------------------------------------------------------

# v1.4 -- Cloud Mehrwert & Skalierung

**Ziel:** Cloud-Version bietet funktionalen Mehrwert

## Performance

-   Read Replicas
-   Query-Optimierung

## Benutzererfahrung

-   Onboarding Wizard
-   Tenant Setup Assistent

## Enterprise Features

-   SLA Monitoring
-   Audit Dashboard
-   Erweiterte Rollenprofile

------------------------------------------------------------------------

# v1.5 -- Monetarisierbare Erweiterungen

**Ziel:** Nachhaltiges Geschäftsmodell ermöglichen

## Premium-Funktionen

-   Gerätehersteller-spezifische Integrationen
-   Compliance Toolkit (ISO-Unterstützung)
-   Erweiterte Analytik
-   Support-Portal Integration

## Strategisch

-   Definition eines Dual-Lizenz-Modells
-   SLA-Struktur
-   Preisstrategie für Cloud-Edition

------------------------------------------------------------------------

# Langfristige Vision

OpenSylab verfolgt eine klare Strategie:

-   Voll funktionsfähige Open-Source-Self-Host-Version
-   Cloud-Version mit erweiterten Sicherheits-, Compliance- und
    Betriebsfunktionen
-   Fokus auf Transparenz, Sicherheit und Skalierbarkeit
-   Klare Trennung zwischen Core-Funktionalität und optionalen
    Enterprise-Erweiterungen
