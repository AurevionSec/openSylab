# OpenSylab - Docker Quick Start

**Version:** 0.6.0 | **Time to deploy:** 5 minutes ⏱️

---

## Prerequisites

- Docker 20.10+ ([Install Docker](https://docs.docker.com/get-docker/))
- Docker Compose 2.0+ (included with Docker Desktop)

---

## Quick Start (3 Steps)

### 1. Clone Repository

```bash
git clone https://github.com/AurevionSec/openSylab.git
cd openSylab
```

### 2. Start Services

```bash
docker compose up -d
```

**Expected output:**
```
[+] Running 3/3
 ✔ Network opensylab_opensylab-network  Created
 ✔ Container opensylab-backend          Started
 ✔ Container opensylab-frontend         Started
```

### 3. Access Application

- **Web UI:** http://localhost
- **API:** http://localhost:8080/api/v1

**Default login:**
- Username: `admin`
- Password: `admin` (⚠️ change immediately!)

---

## Essential Commands

```bash
# View logs
docker compose logs -f

# Stop services
docker compose down

# Restart services
docker compose restart

# Rebuild after code changes
docker compose up -d --build

# Remove everything (including data!)
docker compose down -v
```

---

## What's Running?

| Container | Port | Description |
|-----------|------|-------------|
| `opensylab-frontend` | 80 | React Web UI (nginx) |
| `opensylab-backend` | 8080 | C++ REST API Server |
| `opensylab-data` (volume) | - | SQLite Database (persistent) |

---

## Troubleshooting

**Port already in use?**

Edit `docker-compose.yml` and change ports:

```yaml
services:
  frontend:
    ports:
      - "3000:80"  # Changed from 80:80
```

**Container won't start?**

```bash
# Check logs
docker compose logs backend
docker compose logs frontend

# Check status
docker compose ps
```

**Database errors?**

```bash
# Reset database (⚠️ deletes all data!)
docker compose down -v
docker compose up -d
```

---

## Full Documentation

For production deployment, TLS setup, backups, and advanced configuration, see:

📘 **[docs/DOCKER.md](docs/DOCKER.md)**

---

## System Requirements

**Minimum:**
- 2 CPU cores
- 2 GB RAM
- 5 GB disk space

**Recommended:**
- 4+ CPU cores
- 4+ GB RAM
- 10+ GB disk space (SSD)

---

## Support

- 🐛 Report issues: https://github.com/AurevionSec/openSylab/issues
- 💬 Discussions: https://github.com/AurevionSec/openSylab/discussions
- 📧 Email: A@Eddelbuet.tel

---

**Ready for production?** See [docs/DOCKER.md](docs/DOCKER.md) for HTTPS, backups, and monitoring setup.
