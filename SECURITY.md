# Security Policy

OpenSylab processes medical laboratory data. We take security reports seriously
and appreciate responsible disclosure.

## Supported versions

Security fixes are provided for the latest released minor version. Older versions
receive fixes only at the maintainers' discretion.

| Version | Supported                |
|---------|--------------------------|
| 1.0.x   | ✅ actively              |
| 0.9.x   | ⚠️ critical fixes only   |
| < 0.9   | ❌                       |

This table is updated with each release.

## Reporting a vulnerability

**Please do not open a public GitHub issue for security vulnerabilities.**

Report privately through GitHub's **[Private vulnerability reporting](https://github.com/AurevionSec/openSylab/security/advisories/new)**
(Security → Advisories → *Report a vulnerability*). This keeps the report
confidential until a fix is available.

Please include, where possible:

- Affected component and version / commit
- A description of the vulnerability and its impact (e.g. authentication bypass,
  audit-trail tampering, patient-data exposure)
- Steps to reproduce or a proof of concept
- Any suggested remediation

## What to expect

- **Acknowledgement:** within 5 business days
- **Initial assessment:** within 10 business days
- **Fix & disclosure:** coordinated with the reporter; we aim to release a fix
  before public disclosure and will credit reporters who wish to be named

## Scope

In scope: the OpenSylab backend (C++), frontend (React/TypeScript), API, and the
default deployment configuration.

Particularly sensitive areas — please prioritise these in reports:

- Authentication and JWT handling (`src/auth/`, `src/api/ApiServer.cpp`)
- RBAC enforcement (role checks on API endpoints)
- Audit-trail integrity (HMAC-SHA256 hash chain, `Database::addAuditEntry`)
- SQL query construction (prepared-statement usage)
- TLS / transport configuration

Out of scope: vulnerabilities in third-party dependencies already tracked by
Dependabot (report those upstream), issues requiring a compromised host or
physical access, and the documented development defaults (`admin/admin`, dev JWT
secret) which must never be used in production.
