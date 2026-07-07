# OpenSylab documentation

Start here to find your way around the docs.

| Guide | What it covers |
|-------|----------------|
| [INSTALL.md](INSTALL.md) | Install & run — Docker and native build-from-source |
| [DOCKER.md](DOCKER.md) | Docker deployment in depth (compose, ports, volumes, production) |
| [SECRET_ROTATION.md](SECRET_ROTATION.md) | Managing and rotating `OPENSYLAB_JWT_SECRET` / `OPENSYLAB_AUDIT_HMAC_KEY` |
| [openapi.yaml](openapi.yaml) | Machine-readable REST API contract (load in Swagger UI / Redoc) |
| [TESTING.md](TESTING.md) | Test suite: backend unit, frontend (Vitest), end-to-end (Playwright) |
| [VERSIONING.md](VERSIONING.md) | Version single-source-of-truth and the release process |

**Also useful:** the [README](../README.md) (overview, quick start, configuration, architecture) · the [ROADMAP](../ROADMAP.md) · the [CHANGELOG](../CHANGELOG.md) · [CONTRIBUTING](../CONTRIBUTING.md) · [SECURITY](../SECURITY.md).

## Quick reference

- **Health check:** `GET /api/v1/health` → `{"status":"ok","service":"opensylab-lims"}`
- **Ports (Docker):** frontend `9090`, backend `9080` · **native default:** backend `8080`
- **Required production secrets:** `OPENSYLAB_JWT_SECRET`, `OPENSYLAB_AUDIT_HMAC_KEY` (≥ 32 chars each; `openssl rand -hex 32`)
- **Default login:** `admin` / `admin` — development only, change immediately
