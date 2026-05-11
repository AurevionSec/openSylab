# OpenSylab Docker Deployment Guide

**Version:** 0.7.0
**Date:** 2026-05-11
**Status:** Production-Ready

---

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [Quick Start](#quick-start)
4. [Architecture](#architecture)
5. [Configuration](#configuration)
6. [Production Deployment](#production-deployment)
7. [Development Setup](#development-setup)
8. [Maintenance](#maintenance)
9. [Troubleshooting](#troubleshooting)

---

## Overview

OpenSylab v0.7+ provides full Docker support for easy deployment and scalability. The Docker setup includes:

- **Backend Container**: C++17 REST API server with SQLite
- **Frontend Container**: React SPA served by nginx
- **Docker Compose**: Full-stack orchestration
- **Persistent Storage**: Named volumes for database
- **Health Checks**: Automatic container health monitoring
- **Security**: Non-root users, minimal attack surface

---

## Prerequisites

### Required Software

- **Docker**: Version 20.10+ ([Install Docker](https://docs.docker.com/get-docker/))
- **Docker Compose**: Version 2.0+ ([Install Compose](https://docs.docker.com/compose/install/))

### System Requirements

**Minimum:**
- CPU: 2 cores
- RAM: 2 GB
- Disk: 5 GB free space

**Recommended:**
- CPU: 4+ cores
- RAM: 4+ GB
- Disk: 10+ GB free space (SSD preferred)

---

## Quick Start

### 1. Clone the Repository

```bash
git clone https://github.com/AurevionSec/openSylab.git
cd openSylab
git checkout v0.7
```

### 2. Start the Stack

```bash
# Build and start all services
docker compose up -d

# View logs
docker compose logs -f
```

### 3. Access the Application

- **Frontend (Web UI)**: http://localhost
- **Backend (API)**: http://localhost:8080/api/v1

### 4. Default Credentials

**First Login:**
- Username: `admin`
- Password: `admin` (⚠️ **Change immediately!**)

---

## Architecture

### Container Architecture

```
┌─────────────────────────────────────────────────────┐
│                    Internet                         │
└──────────────────┬──────────────────────────────────┘
                   │
                   │ Port 80
                   ▼
┌──────────────────────────────────────────────────────┐
│          Frontend Container (nginx)                  │
│  ┌────────────────────────────────────────────────┐  │
│  │  React SPA (Vite build)                        │  │
│  │  - Dashboard                                   │  │
│  │  - Sample Management                           │  │
│  │  - Order Management                            │  │
│  │  - Result Management                           │  │
│  └────────────────────────────────────────────────┘  │
└──────────────────┬──────────────────────────────────┘
                   │ API Proxy
                   │ /api/* → backend:8080
                   ▼
┌──────────────────────────────────────────────────────┐
│          Backend Container (C++ API)                 │
│  ┌────────────────────────────────────────────────┐  │
│  │  OpenSylab API Server                          │  │
│  │  - REST API (JWT Auth)                         │  │
│  │  - Business Logic                              │  │
│  │  - Database Access                             │  │
│  └────────────────┬───────────────────────────────┘  │
│                   │                                   │
│                   ▼                                   │
│  ┌────────────────────────────────────────────────┐  │
│  │  SQLite Database                               │  │
│  │  (Persistent Volume)                           │  │
│  └────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────┘
```

### Multi-Stage Builds

Both containers use **multi-stage builds** to minimize image size:

1. **Build Stage**: Compiles code, installs dependencies
2. **Runtime Stage**: Copies only necessary artifacts, runs as non-root user

**Benefits:**
- Smaller images (backend: ~150 MB, frontend: ~30 MB)
- Faster deployments
- Reduced attack surface

---

## Configuration

### Environment Variables

#### Backend Container

| Variable | Default | Description |
|----------|---------|-------------|
| `OPENSYLAB_DB_PATH` | `/app/data/opensylab.db` | SQLite database path |
| `OPENSYLAB_API_PORT` | `8080` | API server port |
| `OPENSYLAB_JWT_SECRET` | (development default) | **REQUIRED** JWT secret key (min. 32 chars) ⚠️ |
| `OPENSYLAB_TLS_CERT` | - | TLS certificate path (optional) |
| `OPENSYLAB_TLS_KEY` | - | TLS private key path (optional) |

**Example** (in `docker-compose.yml`):

```yaml
services:
  backend:
    environment:
      - OPENSYLAB_DB_PATH=/app/data/opensylab.db
      - OPENSYLAB_API_PORT=8080
      - OPENSYLAB_JWT_SECRET=your-secure-random-secret-min-32-characters
```

**⚠️ IMPORTANT - JWT Secret Security:**

The `OPENSYLAB_JWT_SECRET` environment variable is **critical for production security**:
- Must be at least 32 characters long
- Use a cryptographically secure random string
- Never commit secrets to version control
- Change the default value immediately in production

**Generate a secure secret:**
```bash
# Linux/macOS
openssl rand -base64 48

# Or use a password generator
head -c 48 /dev/urandom | base64
```

#### Frontend Container

The frontend is a static build served by nginx. Configuration is done via `nginx.conf`.

### Persistent Data

Database data is stored in a **named Docker volume**:

```yaml
volumes:
  opensylab-data:
    driver: local
```

**Location on host:**
- Linux: `/var/lib/docker/volumes/opensylab_opensylab-data/_data/`
- macOS: `~/Library/Containers/com.docker.docker/Data/vms/0/`
- Windows: `\\wsl$\docker-desktop-data\version-pack-data\community\docker\volumes\`

### Port Mapping

| Service | Container Port | Host Port | Description |
|---------|----------------|-----------|-------------|
| Frontend | 80 | 80 | Web UI (nginx) |
| Backend | 8080 | 8080 | REST API |

**Change host ports** in `docker-compose.yml`:

```yaml
services:
  frontend:
    ports:
      - "3000:80"  # Frontend now on localhost:3000
  backend:
    ports:
      - "9000:8080"  # Backend now on localhost:9000
```

---

## Production Deployment

### 1. Enable TLS/HTTPS

**Generate certificates:**

```bash
# Generate self-signed certificate (development)
mkdir -p certs
openssl req -x509 -newkey rsa:4096 -keyout certs/server.key -out certs/server.crt -days 365 -nodes

# Or use Let's Encrypt (production)
certbot certonly --standalone -d yourdomain.com
cp /etc/letsencrypt/live/yourdomain.com/fullchain.pem certs/server.crt
cp /etc/letsencrypt/live/yourdomain.com/privkey.pem certs/server.key
```

**Update `docker-compose.yml`:**

```yaml
services:
  backend:
    volumes:
      - ./certs:/app/certs:ro
    environment:
      - OPENSYLAB_TLS_CERT=/app/certs/server.crt
      - OPENSYLAB_TLS_KEY=/app/certs/server.key
```

### 2. Use Reverse Proxy (Recommended)

For production, use **nginx or Traefik** as a reverse proxy:

```yaml
# docker-compose.prod.yml
services:
  proxy:
    image: nginx:latest
    ports:
      - "443:443"
      - "80:80"
    volumes:
      - ./nginx-proxy.conf:/etc/nginx/nginx.conf:ro
      - ./certs:/etc/nginx/certs:ro
    depends_on:
      - frontend
      - backend
```

**Example `nginx-proxy.conf`:**

```nginx
server {
    listen 443 ssl http2;
    server_name opensylab.example.com;

    ssl_certificate /etc/nginx/certs/server.crt;
    ssl_certificate_key /etc/nginx/certs/server.key;

    location / {
        proxy_pass http://frontend:80;
    }

    location /api/ {
        proxy_pass http://backend:8080;
    }
}
```

### 3. Database Backups

**Automated backup script:**

```bash
#!/bin/bash
# backup-db.sh
BACKUP_DIR=/backups/opensylab
DATE=$(date +%Y%m%d_%H%M%S)

# Create backup directory
mkdir -p $BACKUP_DIR

# Backup SQLite database
docker exec opensylab-backend sqlite3 /app/data/opensylab.db ".backup /app/data/backup_${DATE}.db"
docker cp opensylab-backend:/app/data/backup_${DATE}.db $BACKUP_DIR/

# Keep only last 7 days
find $BACKUP_DIR -name "backup_*.db" -mtime +7 -delete
```

**Add to crontab:**

```bash
# Daily backup at 2 AM
0 2 * * * /path/to/backup-db.sh
```

### 4. Resource Limits

**Prevent resource exhaustion:**

```yaml
services:
  backend:
    deploy:
      resources:
        limits:
          cpus: '2.0'
          memory: 2G
        reservations:
          cpus: '1.0'
          memory: 1G

  frontend:
    deploy:
      resources:
        limits:
          cpus: '0.5'
          memory: 512M
        reservations:
          cpus: '0.25'
          memory: 256M
```

### 5. Logging

**Configure centralized logging:**

```yaml
services:
  backend:
    logging:
      driver: "json-file"
      options:
        max-size: "10m"
        max-file: "3"
```

**View logs:**

```bash
# All services
docker compose logs -f

# Specific service
docker compose logs -f backend

# Since specific time
docker compose logs --since 2h backend
```

---

## Development Setup

### Hot Reload Development

For development with hot reload, use **bind mounts** instead of volumes:

```yaml
# docker-compose.dev.yml
services:
  backend:
    volumes:
      - ./src:/app/src:ro
      - ./include:/app/include:ro
      - opensylab-build:/app/build  # Cache build artifacts
    command: /bin/bash -c "cd /app/build && cmake .. && make && ./bin/OpenSylab --api --api-port 8080"

  frontend:
    volumes:
      - ./frontend:/app
      - /app/node_modules  # Prevent overwriting node_modules
    command: npm run dev
    ports:
      - "5173:5173"  # Vite dev server
```

**Start development stack:**

```bash
docker compose -f docker-compose.dev.yml up
```

### Running Tests

```bash
# Backend tests
docker compose run --rm backend /app/opensylab_tests

# Frontend tests (when implemented)
docker compose run --rm frontend npm test
```

---

## Maintenance

### Updating OpenSylab

```bash
# Pull latest code
git pull origin v0.7

# Rebuild containers
docker compose build --no-cache

# Restart services
docker compose up -d

# Verify health
docker compose ps
```

### Viewing Logs

```bash
# All services
docker compose logs -f

# Last 100 lines from backend
docker compose logs --tail=100 backend

# Follow specific service
docker compose logs -f frontend
```

### Database Access

```bash
# Enter backend container
docker compose exec backend /bin/bash

# Access SQLite database
sqlite3 /app/data/opensylab.db

# Run SQL query
sqlite3 /app/data/opensylab.db "SELECT * FROM samples LIMIT 10;"
```

### Container Shell Access

```bash
# Backend (Debian-based)
docker compose exec backend /bin/bash

# Frontend (Alpine-based)
docker compose exec frontend /bin/sh
```

### Cleanup

```bash
# Stop and remove containers
docker compose down

# Remove containers, networks, and volumes
docker compose down -v

# Remove unused images
docker image prune -a

# Full cleanup (⚠️ removes ALL unused Docker data)
docker system prune -a --volumes
```

---

## Troubleshooting

### Container Won't Start

**Check logs:**

```bash
docker compose logs backend
docker compose logs frontend
```

**Common issues:**
- Port already in use → Change port in `docker-compose.yml`
- Permission denied → Run with `sudo` or add user to `docker` group
- Out of memory → Increase Docker memory limit in Docker Desktop settings

### Database Errors

**Reset database:**

```bash
# ⚠️ This deletes all data!
docker compose down -v
docker compose up -d
```

**Restore from backup:**

```bash
docker cp backup.db opensylab-backend:/app/data/opensylab.db
docker compose restart backend
```

### Frontend Can't Reach Backend

**Check nginx proxy configuration:**

```bash
docker compose exec frontend cat /etc/nginx/conf.d/default.conf
```

**Verify network connectivity:**

```bash
docker compose exec frontend ping backend
docker compose exec frontend curl http://backend:8080/api/v1/health
```

### Health Check Failing

**Check health status:**

```bash
docker compose ps
```

**Manual health check:**

```bash
# Backend
curl http://localhost:8080/api/v1/health

# Frontend
curl http://localhost/
```

---

## Advanced Topics

### PostgreSQL Integration (Future)

For PostgreSQL support (planned for v0.9+):

```yaml
services:
  postgres:
    image: postgres:16-alpine
    environment:
      POSTGRES_DB: opensylab
      POSTGRES_USER: opensylab
      POSTGRES_PASSWORD: changeme
    volumes:
      - postgres-data:/var/lib/postgresql/data

  backend:
    environment:
      - OPENSYLAB_DB_TYPE=postgres
      - OPENSYLAB_DB_HOST=postgres
      - OPENSYLAB_DB_NAME=opensylab
```

### Kubernetes Deployment (Future)

See `docs/KUBERNETES.md` (planned for v1.0+).

---

## Support

- **GitHub Issues**: https://github.com/AurevionSec/openSylab/issues
- **Discussions**: https://github.com/AurevionSec/openSylab/discussions
- **Email**: A@Eddelbuet.tel

---

**Last Updated:** 2026-02-11
**OpenSylab Version:** v0.7.0
**Document Version:** 1.2
