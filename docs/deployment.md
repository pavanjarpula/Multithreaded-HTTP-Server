# Deployment Guide

## Option 1 — Direct Linux Deployment

### Prerequisites

- Linux VPS (Ubuntu 20.04+ recommended)
- g++ with C++17 support
- CMake 3.16+
- Ninja or Make

### Steps

```bash
# 1. Clone the repository
git clone https://github.com/yourname/cpp-http-server.git
cd cpp-http-server

# 2. Install dependencies (Ubuntu/Debian)
sudo apt update
sudo apt install -y g++ cmake ninja-build

# 3. Build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 4. Run tests
./build/tests/server_tests

# 5. Run the server
./build/http_server --port 8080 --threads 4
```

### Configure as a systemd Service

```bash
# Copy binary and static files to /opt
sudo mkdir -p /opt/http-server
sudo cp build/http_server /opt/http-server/
sudo cp -r public /opt/http-server/

# Install systemd unit
sudo cp deploy/http-server.service /etc/systemd/system/
sudo systemctl daemon-reload

# Start the service
sudo systemctl start http-server

# Enable auto-start on boot
sudo systemctl enable http-server

# Check status
sudo systemctl status http-server

# View logs
sudo journalctl -u http-server -f

# Restart after changes
sudo systemctl restart http-server

# Stop the server
sudo systemctl stop http-server
```

---

## Option 2 — Docker Deployment

### Prerequisites

- Docker installed
- Docker Compose (optional)

### Build and Run

```bash
# Build the image
docker build -t cpp-http-server .

# Run the container
docker run -d -p 8080:8080 --name http-server cpp-http-server

# Check logs
docker logs -f http-server

# Stop
docker stop http-server

# Remove
docker rm http-server
```

### Using Docker Compose

```bash
# Start
docker compose up -d

# Logs
docker compose logs -f

# Stop
docker compose down
```

---

## Optional: Reverse Proxy for HTTPS

The server does not implement TLS. Use a reverse proxy for HTTPS:

```
Internet
   │ HTTPS
   ▼
Nginx / Caddy (reverse proxy)
   │ HTTP
   ▼
C++ HTTP Server (port 8080)
```

### Nginx Example

```nginx
server {
    listen 443 ssl;
    server_name example.com;

    ssl_certificate /etc/letsencrypt/live/example.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/example.com/privkey.pem;

    location / {
        proxy_pass http://127.0.0.1:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

### Caddy Example

```
example.com {
    reverse_proxy localhost:8080
}
```

Caddy auto-configures HTTPS via Let's Encrypt.
