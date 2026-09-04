# Deployment Guide

## Local Development

```
Build → Run → localhost:8080
```

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/http_server --port 8080
```

Open `http://localhost:8080/` in your browser.

---

## Public Deployment (Render.com)

### How It Works

```
git push → GitHub → Render webhook → Docker build → Deploy → Public URL
```

### One-Time Setup

1. **Push code to GitHub**
   ```bash
   git add .
   git commit -m "Initial deployment"
   git push
   ```

2. **Connect to Render**
   - Go to [render.com](https://render.com) and sign up (free)
   - Click **New** → **Web Service**
   - Connect your GitHub repository
   - Render detects the `render.yaml` automatically

3. **Configuration** (already set in `render.yaml`)
   - Runtime: Docker
   - Port: Reads `PORT` environment variable
   - Health check: `/health`

4. **Deploy**
   - Click **Create Web Service**
   - Render builds the Docker image and deploys
   - Your public URL appears (e.g., `https://cpp-http-server.onrender.com`)

### Automatic Redeployment

After initial setup, every `git push` to `main` triggers automatic redeployment:

```
git add .
git commit -m "Update feature"
git push
  ↓
Render detects push
  ↓
Builds new Docker image
  ↓
Deploys new version
  ↓
Public URL updated
```

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `PORT` | `8080` | Server listen port (set by Render) |
| `THREADS` | `4` | Worker thread count |

The C++ server reads `PORT` from the environment. On Render, this is set automatically.

---

## Docker Deployment

### Build and Run

```bash
docker build -t cpp-http-server .
docker run -p 8080:8080 cpp-http-server
```

### With Custom Port

```bash
docker run -p 3000:3000 -e PORT=3000 cpp-http-server
```

### Docker Compose

```bash
docker compose up -d
```

---

## systemd Deployment (Linux VPS)

```bash
# Copy files
sudo cp build/http_server /opt/http-server/
sudo cp -r public /opt/http-server/
sudo cp deploy/http-server.service /etc/systemd/system/

# Enable and start
sudo systemctl daemon-reload
sudo systemctl start http-server
sudo systemctl enable http-server

# Commands
sudo systemctl status http-server
sudo systemctl restart http-server
sudo systemctl stop http-server
sudo journalctl -u http-server -f
```

---

## HTTPS via Reverse Proxy

```
Internet → Nginx (HTTPS) → C++ Server (HTTP localhost:8080)
```

```nginx
server {
    listen 443 ssl;
    server_name yourdomain.com;

    ssl_certificate /etc/letsencrypt/live/yourdomain.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/yourdomain.com/privkey.pem;

    location / {
        proxy_pass http://127.0.0.1:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
}
```

---

## Health Endpoint

```
GET /health
```

Response:
```json
{"status": "ok"}
```

Used by deployment platforms to verify the application is running.
