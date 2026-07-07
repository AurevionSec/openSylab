# 🚀 OpenSylab Roadmap

OpenSylab is evolving from a capable open-source LIMS solution into a
professional, cloud-ready platform with a clear separation between the
Self-Host Edition and a future Cloud Edition.

This roadmap describes the planned development steps from v0.7 to v1.5.

------------------------------------------------------------------------

# v0.7 -- Workflow Completeness & UI
**Goal:** Complete coverage of the laboratory workflow in the frontend

## Frontend
-   Full CRUD operations for Samples, Orders, and Results
-   Result Entry Form with validation against reference ranges
-   Barcode scanner integration for rapid data capture
-   Data visualization (charts) in the dashboard
-   CSV Import UI in the frontend

## API & Documentation
-   Completion of all required DELETE endpoints
-   Swagger UI & OpenAPI 3.0 specification for developers
-   Continued integration of laboratory instruments

------------------------------------------------------------------------

# v0.8 -- Security & Engineering Stabilization

**Goal:** Establish a solid technical trust foundation

## Security Improvements

-   Removal of all default secrets
-   Enforced secret generation on first start
-   Password hashing with Argon2
-   Session hardening
-   Basic rate limiting

## Engineering & Quality

-   CI/CD pipeline
-   Unit test coverage reporting
-   Static code analysis (backend & frontend)
-   Dependency security scanning
-   Auto-generated OpenAPI specification

## Documentation

-   Architecture diagram
-   Security hardening guide
-   Deployment guide for self-hosting

------------------------------------------------------------------------

# v0.9 -- Architecture Modernization

**Goal:** Prepare the codebase for scaling and cloud readiness

## Database

-   Official PostgreSQL support
-   Database migration framework
-   Clean DB abstraction layer

## Logging & Audit

-   Tamper-proof audit logs (hash chain)
-   Structured JSON logs

## Configuration

-   Environment-based configuration
-   Preparation for external secret management (Vault/KMS)

## API

-   Versioned REST API
-   Initial SDK support (Python)

------------------------------------------------------------------------

# v1.0 -- Stable Open-Source Edition

**Goal:** Production-ready self-host version (research use)

## Security

-   Extended RBAC
-   Optional multi-factor authentication
-   Improved session management

## Reporting

-   Audit report export (PDF)
-   User and sample export

## Integration

-   Stabilization of HL7 support
-   Improved CSV import
-   Complete API documentation

## Operations

-   Documented backup strategy
-   Tested restore procedure

------------------------------------------------------------------------

# v1.1 -- Multi-Tenant Foundations

**Goal:** Prepare multi-tenancy

## Architecture

-   Tenant ID at the data layer
-   Option for isolated databases per tenant
-   Tenant-specific configuration

## Security

-   Preparation for tenant-specific encryption keys
-   Storage abstraction (S3-compatible)

------------------------------------------------------------------------

# v1.2 -- Compliance & Data Protection

**Goal:** Preparation for regulatory use in the German market

## Data Protection

-   Data export in accordance with GDPR (Art. 15)
-   Deletion concept
-   Configurable logging retention

## Security

-   Enforced TLS
-   Login attempt monitoring
-   Optional IP whitelist

## Documentation

-   Technical and organizational measures (TOM) template
-   Threat model
-   Risk analysis

------------------------------------------------------------------------

# v1.3 -- Cloud Edition Technical Foundation

**Goal:** Technical foundation for the future cloud version

## Tenant Lifecycle

-   Tenant creation & management
-   Tenant-specific limits

## Monitoring

-   Health dashboard
-   System metrics

## Backup

-   Automated backups
-   Documented restore procedure

## Enterprise Security

-   OIDC / SSO integration
-   Mandatory MFA for admins

------------------------------------------------------------------------

# v1.4 -- Cloud Value & Scaling

**Goal:** Cloud version delivers tangible functional value

## Performance

-   Read replicas
-   Query optimization

## User Experience

-   Onboarding wizard
-   Tenant setup assistant

## Enterprise Features

-   SLA monitoring
-   Audit dashboard
-   Extended role profiles

------------------------------------------------------------------------

# v1.5 -- Monetizable Extensions

**Goal:** Enable a sustainable business model

## Premium Features

-   Device manufacturer-specific integrations
-   Compliance toolkit (ISO support)
-   Advanced analytics
-   Support portal integration

## Strategic

-   Definition of a dual-license model
-   SLA structure
-   Pricing strategy for the Cloud Edition

------------------------------------------------------------------------

# Long-Term Vision

OpenSylab pursues a clear strategy:

-   Fully functional open-source self-host version
-   Cloud version with extended security, compliance, and operations features
-   Focus on transparency, security, and scalability
-   Clear separation between core functionality and optional enterprise
    extensions
