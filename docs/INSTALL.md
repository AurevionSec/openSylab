# OpenSylab v0.6.0 - Installation und Deployment

## Überblick

OpenSylab kann auf verschiedene Arten installiert werden:

1. **🐳 Docker (Empfohlen)** - Einfachste und schnellste Installation
2. **📦 Native Installation** - Für Entwicklung und direkte System-Integration
3. **☁️ Cloud Deployment** - Für Produktionsumgebungen

## Systemanforderungen

### Minimum
- **CPU**: Dual-Core (2 GHz+)
- **RAM**: 2 GB
- **Festplatte**: 500 MB freier Speicher
- **Betriebssystem**: Linux, Windows 10/11, macOS

### Empfohlen für Produktion
- **CPU**: Quad-Core (2.5 GHz+)
- **RAM**: 4 GB+
- **Festplatte**: SSD mit 10 GB+ freiem Speicher
- **Netzwerk**: LAN/WAN mit stabiler Verbindung
- **Betriebssystem**: Linux (Ubuntu 22.04+ oder Debian 11+)

---

## 🐳 Installation mit Docker (Empfohlen)

Docker ist die **empfohlene Installationsmethode** für OpenSylab v0.6.0. Sie bietet:

- ✅ Keine manuelle Abhängigkeiten-Installation
- ✅ Konsistente Umgebung auf allen Betriebssystemen
- ✅ Einfache Updates und Rollbacks
- ✅ Isolation und Sicherheit
- ✅ Production-ready Setup

### Voraussetzungen

- **Docker**: Version 20.10+ ([Installation](https://docs.docker.com/get-docker/))
- **Docker Compose**: Version 2.0+ (meist mit Docker enthalten)

#### Docker installieren

**Linux (Ubuntu/Debian):**
```bash
# Docker installieren
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh

# Benutzer zur Docker-Gruppe hinzufügen
sudo usermod -aG docker $USER
newgrp docker

# Docker Compose installieren (falls nicht vorhanden)
sudo apt install docker-compose-plugin
```

**Windows/macOS:**
- [Docker Desktop](https://www.docker.com/products/docker-desktop/) herunterladen und installieren

### Quick Start mit Docker

```bash
# Repository klonen
git clone https://github.com/AurevionSec/openSylab.git
cd openSylab

# Docker Container starten
docker compose up -d

# Status prüfen
docker compose ps
```

**Das war's!** OpenSylab läuft jetzt:
- **Frontend**: http://localhost:5173
- **Backend API**: http://localhost:8080
- **Default Login**: admin / admin ⚠️ **MUSS geändert werden!**

### Docker Compose Konfiguration

Die `docker-compose.yml` definiert zwei Services:

```yaml
services:
  backend:
    # C++ Backend mit API-Server
    ports:
      - "8080:8080"
    volumes:
      - ./data:/app/data  # Datenbank-Persistenz

  frontend:
    # React Frontend
    ports:
      - "5173:5173"
    depends_on:
      - backend
```

### Docker-Befehle

```bash
# Container starten
docker compose up -d

# Logs anzeigen
docker compose logs -f

# Backend-Logs
docker compose logs -f backend

# Frontend-Logs
docker compose logs -f frontend

# Container stoppen
docker compose stop

# Container stoppen und entfernen
docker compose down

# Container neu starten
docker compose restart

# Container neu bauen (nach Code-Änderungen)
docker compose up -d --build
```

### Daten-Persistenz

Die SQLite-Datenbank wird im Volume `./data` gespeichert:

```bash
# Datenbank-Backup erstellen
docker compose exec backend sqlite3 /app/data/opensylab.db ".backup /app/data/backup.db"

# Oder mit Host-Befehlen
cp data/opensylab.db data/backup_$(date +%Y%m%d_%H%M%S).db
```

### Production Deployment mit Docker

Für Produktionsumgebungen:

1. **HTTPS/TLS konfigurieren** (siehe [TLS_SETUP.md](TLS_SETUP.md))
2. **Secrets externalisieren** (Environment Variables)
3. **Reverse Proxy** (nginx/traefik) verwenden
4. **Regelmäßige Backups** einrichten
5. **Monitoring** aktivieren

**Beispiel Production Compose:**

```yaml
services:
  backend:
    image: opensylab/backend:0.6.0
    restart: unless-stopped
    environment:
      - OPENSYLAB_DB_PATH=/app/data/opensylab.db
      - OPENSYLAB_JWT_SECRET=${JWT_SECRET}  # Aus .env
      - OPENSYLAB_TLS_CERT=/app/certs/cert.pem
      - OPENSYLAB_TLS_KEY=/app/certs/key.pem
    volumes:
      - ./data:/app/data
      - ./certs:/app/certs:ro
    networks:
      - opensylab-net

  frontend:
    image: opensylab/frontend:0.6.0
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

## 📦 Native Installation (Für Entwickler)

Für Entwicklung oder wenn Docker nicht verfügbar ist.

### Abhängigkeiten

#### Benötigte Software

- **C++ Compiler**: GCC 9+, Clang 10+, oder MSVC 2019+
- **CMake**: 3.15+
- **SQLite3**: Development Libraries
- **OpenSSL**: 3.x Development Libraries
- **Node.js**: 18+
- **npm**: 9+
- **Git**

### Installation der Abhängigkeiten

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
1. [Visual Studio 2019+](https://visualstudio.microsoft.com/) mit "Desktop development with C++"
2. [Node.js](https://nodejs.org/) installieren
3. SQLite3 und OpenSSL über vcpkg:
```powershell
vcpkg install sqlite3:x64-windows openssl:x64-windows
```

### Repository klonen

```bash
git clone https://github.com/AurevionSec/openSylab.git
cd openSylab
```

### Backend kompilieren

```bash
# Build-Verzeichnis erstellen
mkdir -p build && cd build

# CMake konfigurieren
cmake ..

# Kompilieren (parallel mit allen CPU-Kernen)
make -j$(nproc)

# Optional: Tests ausführen
./bin/opensylab_tests

# Zurück zum Projektverzeichnis
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

### Frontend einrichten

```bash
cd frontend

# Dependencies installieren
npm install

# Environment-Datei erstellen
cp .env.example .env.development

# .env.development editieren (optional)
# VITE_API_URL=http://localhost:8080/api/v1
```

### Anwendung starten

**Terminal 1 - Backend:**
```bash
./build/bin/OpenSylab --api --api-port 8080
```

**Terminal 2 - Frontend:**
```bash
cd frontend
npm run dev
```

**Zugriff:**
- Frontend: http://localhost:5173
- API: http://localhost:8080/api/v1
- Default Login: admin / admin

### Development mit HTTPS/TLS

Für lokale HTTPS-Entwicklung:

```bash
# Self-signed Zertifikat erstellen
cd certs
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes

# Backend mit TLS starten
./build/bin/OpenSylab --api --api-port 8080 \
    --tls --tls-cert certs/cert.pem --tls-key certs/key.pem
```

---

## 🔧 Konfiguration

### Environment Variables

```bash
# Backend
export OPENSYLAB_DB_PATH=/pfad/zur/datenbank.db
export OPENSYLAB_JWT_SECRET=your-super-secret-key-here
export OPENSYLAB_TLS_CERT=/pfad/zu/cert.pem
export OPENSYLAB_TLS_KEY=/pfad/zu/key.pem

# Frontend
export VITE_API_URL=http://localhost:8080/api/v1
```

### Konfigurationsdatei (v0.8.0+)

Ab v0.8.0 wird eine Konfigurationsdatei unterstützt:

```ini
# opensylab.conf (geplant für v0.8.0)
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

## 🧪 Tests ausführen

### Backend Tests

```bash
cd build

# Alle Tests
make test

# Oder mit ctest
ctest --output-on-failure

# Oder direkt
./bin/opensylab_tests
```

**Erwartetes Ergebnis:**
```
✓ Passed: 62
✗ Failed: 0
Total:   62
```

### Frontend Tests (v0.8.0+)

```bash
cd frontend

# Unit Tests
npm test

# Coverage Report
npm run test:coverage
```

---

## 🚀 Production Deployment

### Checkliste vor Production Deployment

- [ ] **Default-Credentials ändern** (admin/admin)
- [ ] **JWT-Secret externalisieren** (nicht hardcoded!)
- [ ] **HTTPS/TLS aktivieren** (kein HTTP in Produktion)
- [ ] **Firewall konfigurieren** (nur Port 443 öffentlich)
- [ ] **Regelmäßige Backups** einrichten
- [ ] **Monitoring** einrichten (Logs, Metriken)
- [ ] **Security Hardening** durchführen
- [ ] **Updates planen** (Security Patches)

⚠️ **WICHTIG**: OpenSylab v0.6.0 verwendet DJB2 Password Hashing (NICHT production-ready!).
Upgrade auf v0.8.0 mit bcrypt/argon2 wird DRINGEND empfohlen!

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
# Service aktivieren
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

## 🔍 Fehlerbehebung

### Docker-Probleme

**Container startet nicht:**
```bash
# Logs prüfen
docker compose logs

# Container-Status
docker compose ps

# Ports prüfen
sudo netstat -tulpn | grep -E '5173|8080'
```

**Port bereits belegt:**
```bash
# Ports in docker-compose.yml ändern
# z.B. "5174:5173" statt "5173:5173"
```

### Native Build-Probleme

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

**CMake-Version zu alt:**
```bash
# Ubuntu - CMake von Kitware
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | sudo apt-key add -
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt update
sudo apt install cmake
```

### Runtime-Probleme

**"Failed to open database":**
```bash
# Schreibrechte prüfen
ls -la opensylab.db

# Datenbankpfad explizit angeben
./build/bin/OpenSylab /tmp/opensylab.db
```

**"Connection refused" beim Frontend:**
```bash
# Backend läuft?
curl http://localhost:8080/api/v1/stats

# CORS-Einstellungen prüfen
# Backend muss Frontend-Origin erlauben
```

**"Invalid credentials":**
- Default: admin / admin
- Password-Hash-Algorithmus prüfen (siehe Troubleshooting in UI_EXTENSIONS_V06.md)
- Datenbank-Inhalt prüfen: `sqlite3 opensylab.db "SELECT * FROM users;"`

---

## 📚 Weitere Ressourcen

- **[README.MD](../README.MD)** - Projektübersicht und Features
- **[ROADMAP.MD](../ROADMAP.MD)** - Entwicklungs-Roadmap
- **[CHANGELOG.md](../CHANGELOG.md)** - Versionshistorie
- **[TODO.md](../TODO.md)** - Geplante Features
- **[UI_EXTENSIONS_V06.md](../frontend/UI_EXTENSIONS_V06.md)** - UI-Dokumentation
- **[TESTING.md](TESTING.md)** - Test-Dokumentation

## 💬 Support

Bei Problemen:

1. **Logs prüfen** (Docker: `docker compose logs`, Native: Console-Output)
2. **Dokumentation lesen** (README, INSTALL, Troubleshooting-Guides)
3. **GitHub Issues** durchsuchen: https://github.com/AurevionSec/openSylab/issues
4. **Neues Issue erstellen** mit:
   - OpenSylab Version
   - Betriebssystem + Version
   - Installationsmethode (Docker/Native)
   - Fehlermeldung (vollständig)
   - Reproduktionsschritte

## 🔄 Updates

### Docker Update

```bash
# Neueste Version pullen
docker compose pull

# Container neu starten
docker compose up -d
```

### Native Update

```bash
# Repository aktualisieren
git pull origin main

# Backend neu kompilieren
cd build
make clean
cmake ..
make -j$(nproc)

# Frontend neu bauen
cd ../frontend
npm install  # Falls neue Dependencies
npm run build
```

---

**Version:** 0.6.0
**Letzte Aktualisierung:** 2026-02-03
**Nächste Version:** 0.8.0 (Production Security & Docker Improvements)
