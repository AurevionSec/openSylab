# OpenSylab v1.1.0 - Installation and Deployment

## Overview

OpenSylab can be installed in several ways:

1. **🐳 Docker (Recommended)** - Simplest and fastest installation
2. **📦 Native Installation** - For development and direct system integration
3. **☁️ Cloud Deployment** - For production environments

## System Requirements

### Minimum
- **CPU**: Dual-Core (2 GHz+)
- **RAM**: 2 GB
- **Disk**: 500 MB free space
- **Operating System**: Linux, Windows 10/11, macOS

### Recommended for Production
- **CPU**: Quad-Core (2.5 GHz+)
- **RAM**: 4 GB+
- **Disk**: SSD with 10 GB+ free space
- **Network**: LAN/WAN with a stable connection
- **Operating System**: Linux (Ubuntu 22.04+ or Debian 11+)

---

## 🐳 Installation with Docker (Recommended)

Docker is the **recommended installation method** for OpenSylab v1.1.0. It offers:

- ✅ No manual dependency installation
- ✅ Consistent environment across all operating systems
- ✅ Easy updates and rollbacks
- ✅ Isolation and security
- ✅ Production-ready setup

### Prerequisites

- **Docker**: Version 20.10+ ([Installation](https://docs.docker.com/get-docker/))
- **Docker Compose**: Version 2.0+ (usually included with Docker)

#### Installing Docker

**Linux (Ubuntu/Debian):**
```bash
# Install Docker
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh

# Add user to the Docker group
sudo usermod -aG docker $USER
newgrp docker

# Install Docker Compose (if not already present)
sudo apt install docker-compose-plugin
```

**Windows/macOS:**
- Download and install [Docker Desktop](https://www.docker.com/products/docker-desktop/)

### Quick Start with Docker

```bash
# Clone the repository
git clone https://github.com/AurevionSec/openSylab.git
cd openSylab

# Start the Docker containers
docker compose up -d

# Check status
docker compose ps
```

**That's it!** OpenSylab is now running (Docker maps to the host ports 9090/9080):
- **Frontend**: http://localhost:9090
- **Backend API**: http://localhost:9080
- **Default Login**: admin / admin ⚠️ **MUST be changed!**

### Docker Compose Configuration

The `docker-compose.yml` defines two services:

```yaml
services:
  backend:
    # C++ backend with API server (container port 8080 → host port 9080)
    ports:
      - "9080:8080"
    volumes:
      - ./data:/app/data  # Database persistence

  frontend:
    # React frontend (nginx on container port 80 → host port 9090)
    ports:
      - "9090:80"
    depends_on:
      - backend
```

### Docker Commands

```bash
# Start containers
docker compose up -d

# Show logs
docker compose logs -f

# Backend logs
docker compose logs -f backend

# Frontend logs
docker compose logs -f frontend

# Stop containers
docker compose stop

# Stop and remove containers
docker compose down

# Restart containers
docker compose restart

# Rebuild containers (after code changes)
docker compose up -d --build
```

### Data Persistence

The SQLite database is stored in the volume `./data`:

```bash
# Create a database backup
docker compose exec backend sqlite3 /app/data/opensylab.db ".backup /app/data/backup.db"

# Or using host commands
cp data/opensylab.db data/backup_$(date +%Y%m%d_%H%M%S).db
```

### Production Deployment with Docker

For production environments:

1. **Configure HTTPS/TLS** (see [README → Configuration](../README.md) and [SECRET_ROTATION.md](SECRET_ROTATION.md))
2. **Externalize secrets** (Environment Variables)
3. **Use a reverse proxy** (nginx/traefik)
4. **Set up regular backups**
5. **Enable monitoring**

**Example Production Compose:**

```yaml
services:
  backend:
    image: opensylab/backend:1.1.0
    restart: unless-stopped
    environment:
      - OPENSYLAB_DB_PATH=/app/data/opensylab.db
      - OPENSYLAB_JWT_SECRET=${JWT_SECRET}  # From .env
      - OPENSYLAB_TLS_CERT=/app/certs/cert.pem
      - OPENSYLAB_TLS_KEY=/app/certs/key.pem
    volumes:
      - ./data:/app/data
      - ./certs:/app/certs:ro
    networks:
      - opensylab-net

  frontend:
    image: opensylab/frontend:1.1.0
    restart: unless-stopped
    environment:
      - VITE_API_URL=https://api.yourdomain.com
    networks:
      - opensylab-net

  nginx:
    image: nginx:alpine
    ports:
      - "443:443"
      - "80:80"
    volumes:
      - ./nginx.conf:/etc/nginx/nginx.conf:ro
      - ./certs:/etc/nginx/certs:ro
    depends_on:
      - backend
      - frontend
    networks:
      - opensylab-net

networks:
  opensylab-net:
    driver: bridge
```

---

## 📦 Native Installation (For Developers)

For development or when Docker is not available.

### Dependencies

#### Required Software

- **C++ Compiler**: GCC 9+, Clang 10+, or MSVC 2019+
- **CMake**: 3.15+
- **SQLite3**: Development Libraries
- **OpenSSL**: 3.x Development Libraries
- **Node.js**: 18+
- **npm**: 9+
- **Git**

### Installing the Dependencies

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install -y build-essential cmake git \
    libsqlite3-dev libssl-dev \
    nodejs npm
```

**Fedora/RHEL:**
```bash
sudo dnf install -y gcc-c++ cmake git \
    sqlite-devel openssl-devel \
    nodejs npm
```

**macOS (Homebrew):**
```bash
brew install cmake sqlite3 openssl node
```

**Windows:**
1. [Visual Studio 2019+](https://visualstudio.microsoft.com/) with "Desktop development with C++"
2. Install [Node.js](https://nodejs.org/)
3. SQLite3 and OpenSSL via vcpkg:
```powershell
vcpkg install sqlite3:x64-windows openssl:x64-windows
```

### Clone the Repository

```bash
git clone https://github.com/AurevionSec/openSylab.git
cd openSylab
```

### Compile the Backend

```bash
# Create build directory
mkdir -p build && cd build

# Configure CMake
cmake ..

# Compile (in parallel with all CPU cores)
make -j$(nproc)

# Optional: run tests
./bin/opensylab_tests

# Back to the project directory
cd ..
```

**Windows (Visual Studio):**
```powershell
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release
cd ..
```

### Set Up the Frontend

```bash
cd frontend

# Install dependencies
npm install

# Create the environment file
cp .env.example .env.development

# Edit .env.development (optional)
# VITE_API_URL=http://localhost:8080/api/v1
```

### Start the Application

**Terminal 1 - Backend:**
```bash
./build/bin/OpenSylab --api --api-port 8080
```

**Terminal 2 - Frontend:**
```bash
cd frontend
npm run dev
```

**Access:**
- Frontend: http://localhost:5173
- API: http://localhost:8080/api/v1
- Default Login: admin / admin

### Development with HTTPS/TLS

For local HTTPS development:

```bash
# Create a self-signed certificate
cd certs
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes

# Start the backend with TLS
./build/bin/OpenSylab --api --api-port 8080 \
    --tls --tls-cert certs/cert.pem --tls-key certs/key.pem
```

---

## 🔧 Configuration

### Environment Variables

```bash
# Backend
export OPENSYLAB_DB_PATH=/path/to/database.db
export OPENSYLAB_JWT_SECRET="$(openssl rand -hex 32)"       # Required in prod (>=32 characters)
export OPENSYLAB_AUDIT_HMAC_KEY="$(openssl rand -hex 32)"   # Required in prod — server won't start otherwise
export OPENSYLAB_CORS_ORIGIN=https://lims.example.org       # allowed frontend origin
export OPENSYLAB_TLS_CERT=/path/to/cert.pem
export OPENSYLAB_TLS_KEY=/path/to/key.pem

# Frontend
export VITE_API_URL=http://localhost:8080/api/v1
```

### Configuration File

OpenSylab supports an optional configuration file:

```ini
# opensylab.conf
[database]
path = /var/lib/opensylab/opensylab.db

[api]
port = 8080
enable_tls = true
tls_cert = /etc/opensylab/certs/cert.pem
tls_key = /etc/opensylab/certs/key.pem

[auth]
jwt_secret_file = /etc/opensylab/secrets/jwt.key
token_expiry = 3600

[cors]
allowed_origins = https://opensylab.yourdomain.com
```

---

## 🧪 Running Tests

### Backend Tests

```bash
cd build

# All tests
make test

# Or with ctest
ctest --output-on-failure

# Or directly
./bin/opensylab_tests
```

**Expected result:**
```
✓ Passed: 235
✗ Failed: 0
Total:   235
```

### Frontend Tests (v0.8.0+)

```bash
cd frontend

# Unit tests
npm test

# Coverage report
npm run test:coverage
```

---

## 🚀 Production Deployment

### Checklist Before Production Deployment

- [ ] **Change default credentials** (admin/admin)
- [ ] **Externalize JWT secret** (not hardcoded!)
- [ ] **Enable HTTPS/TLS** (no HTTP in production)
- [ ] **Configure the firewall** (only port 443 public)
- [ ] **Set up regular backups**
- [ ] **Set up monitoring** (logs, metrics)
- [ ] **Perform security hardening**
- [ ] **Plan updates** (security patches)

✅ **Password Security**: OpenSylab v0.7.0+ uses PBKDF2-HMAC-SHA256 (210,000 iterations) — production-ready.

### Systemd Service (Linux)

```ini
# /etc/systemd/system/opensylab-backend.service
[Unit]
Description=OpenSylab Backend API
After=network.target

[Service]
Type=simple
User=opensylab
Group=opensylab
WorkingDirectory=/opt/opensylab
ExecStart=/opt/opensylab/build/bin/OpenSylab --api --api-port 8080 --tls
Restart=on-failure
RestartSec=10s

Environment="OPENSYLAB_DB_PATH=/var/lib/opensylab/opensylab.db"
Environment="OPENSYLAB_JWT_SECRET_FILE=/etc/opensylab/secrets/jwt.key"

[Install]
WantedBy=multi-user.target
```

```bash
# Enable the service
sudo systemctl enable opensylab-backend
sudo systemctl start opensylab-backend
sudo systemctl status opensylab-backend
```

### Nginx Reverse Proxy

```nginx
# /etc/nginx/sites-available/opensylab
server {
    listen 443 ssl http2;
    server_name opensylab.yourdomain.com;

    ssl_certificate /etc/letsencrypt/live/opensylab.yourdomain.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/opensylab.yourdomain.com/privkey.pem;

    # Frontend
    location / {
        proxy_pass http://localhost:5173;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_cache_bypass $http_upgrade;
    }

    # Backend API
    location /api/ {
        proxy_pass http://localhost:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}

# HTTP -> HTTPS Redirect
server {
    listen 80;
    server_name opensylab.yourdomain.com;
    return 301 https://$server_name$request_uri;
}
```

---

## 🔍 Troubleshooting

### Docker Problems

**Container won't start:**
```bash
# Check logs
docker compose logs

# Container status
docker compose ps

# Check ports
sudo netstat -tulpn | grep -E '9090|9080'
```

**Port already in use:**
```bash
# Change the host ports in docker-compose.yml
# e.g. "9091:80" instead of "9090:80" (frontend) or "9081:8080" (backend)
```

### Native Build Problems

**"sqlite3.h not found":**
```bash
# Ubuntu/Debian
sudo apt install libsqlite3-dev

# Fedora
sudo dnf install sqlite-devel
```

**"openssl/ssl.h not found":**
```bash
# Ubuntu/Debian
sudo apt install libssl-dev

# Fedora
sudo dnf install openssl-devel
```

**CMake version too old:**
```bash
# Ubuntu - CMake from Kitware
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | sudo apt-key add -
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt update
sudo apt install cmake
```

### Runtime Problems

**"Failed to open database":**
```bash
# Check write permissions
ls -la opensylab.db

# Specify the database path explicitly
./build/bin/OpenSylab /tmp/opensylab.db
```

**"Connection refused" from the frontend:**
```bash
# Is the backend running?
curl http://localhost:8080/api/v1/stats

# Check CORS settings
# The backend must allow the frontend origin
```

**"Invalid credentials":**
- Default: admin / admin
- Check the database: `sqlite3 opensylab.db "SELECT username, password_hash FROM users;"`
- Check the database contents: `sqlite3 opensylab.db "SELECT * FROM users;"`

---

## 📚 Further Resources

- **[README.md](../README.md)** - Project overview and features
- **[ROADMAP.MD](../ROADMAP.MD)** - Development roadmap
- **[CHANGELOG.md](../CHANGELOG.md)** - Version history
- **[TODO.md](../TODO.md)** - Planned features
- **[UI_EXTENSIONS_V06.md](../frontend/UI_EXTENSIONS_V06.md)** - UI documentation
- **[TESTING.md](TESTING.md)** - Test documentation

## 💬 Support

If you run into problems:

1. **Check the logs** (Docker: `docker compose logs`, Native: console output)
2. **Read the documentation** (README, INSTALL, troubleshooting guides)
3. **Search the GitHub Issues**: https://github.com/AurevionSec/openSylab/issues
4. **Create a new issue** with:
   - OpenSylab version
   - Operating system + version
   - Installation method (Docker/Native)
   - Error message (complete)
   - Reproduction steps

## 🔄 Updates

### Docker Update

```bash
# Pull the latest version
docker compose pull

# Restart the containers
docker compose up -d
```

### Native Update

```bash
# Update the repository
git pull origin main

# Recompile the backend
cd build
make clean
cmake ..
make -j$(nproc)

# Rebuild the frontend
cd ../frontend
npm install  # If there are new dependencies
npm run build
```

---

**Version:** 1.1.0
**Last updated:** 2026-07-05
**Roadmap:** see [ROADMAP.md](../ROADMAP.md) — planned items include a PostgreSQL backend and multi-site support.
