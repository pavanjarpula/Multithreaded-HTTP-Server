# C++ Concurrent HTTP Server

A multithreaded HTTP/1.1 server built from scratch using C++17 and low-level TCP sockets.

---

## Live Demo

| | URL |
|---|---|
| Landing Page | `https://yourusername.github.io/cpp-http-server/` |
| Dashboard | `https://yourusername.github.io/cpp-http-server/dashboard.html` |
| Metrics API | `http://localhost:8080/metrics` (requires running server) |

---

## Quick Start (Run Locally)

```bash
# Build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run
./build/http_server --port 8080 --threads 4

# Open in browser
# http://localhost:8080/
# http://localhost:8080/dashboard
```

---

## Deploy to GitHub Pages (Free Permanent URL)

### Step 1 — Push to GitHub

```bash
git init
git add .
git commit -m "Initial commit"
git remote add origin https://github.com/yourusername/cpp-http-server.git
git push -u origin main
```

### Step 2 — Enable GitHub Pages

1. Go to your repo on GitHub
2. **Settings** → **Pages**
3. Source: **Deploy from a branch**
4. Branch: **main**, folder: **/public**
5. Click **Save**

Your landing page is now live at:
```
https://yourusername.github.io/cpp-http-server/
```

### Step 3 — Run C++ Server Locally (for Dashboard Metrics)

The landing page works permanently on GitHub Pages. The dashboard needs the C++ server running to show live metrics.

**Option A — Run locally:**
```bash
./build/http_server --port 8080
```
Then open `http://localhost:8080/dashboard` — metrics update live.

**Option B — Expose locally via ngrok (shareable link):**
```powershell
# Install ngrok from https://ngrok.com/download
ngrok http 8080
```
This gives you a public URL like `https://abc123.ngrok-free.app`.

In the dashboard, enter that URL in the "Server URL" input and click **Connect**.

---

## Deploy to a VPS (24/7, No PC Needed)

### Option A — Docker

```bash
docker build -t cpp-http-server .
docker run -d -p 8080:8080 cpp-http-server
```

### Option B — systemd (Linux)

```bash
sudo cp deploy/http-server.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl start http-server
sudo systemctl enable http-server
```

### Option C — Reverse Proxy for HTTPS

```
Internet → Nginx (HTTPS) → C++ Server (HTTP localhost:8080)
```

See [docs/deployment.md](docs/deployment.md) for full instructions.

---

## API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/` | Landing page |
| GET | `/dashboard` | Live monitoring dashboard |
| GET | `/health` | Health check (`{"status":"ok"}`) |
| GET | `/hello` | Hello world |
| GET | `/metrics` | Server metrics (JSON) |
| POST | `/echo` | Echo request body |

---

## Architecture

```
Client → TCP Socket → Accept Loop → Blocking Queue → Thread Pool
                                                          │
                              ┌───────────────────────────┘
                              ▼
                    HTTP Parser → Router → Handler → Response
```

See [docs/architecture.md](docs/architecture.md) for detailed explanation.

---

## Features

- TCP socket communication (cross-platform: Windows + Linux)
- HTTP/1.1 request parsing (method, path, headers, body)
- Fixed-size thread pool with blocking task queue
- URL routing with 404/405 handling
- Static file serving with MIME detection
- Thread-safe structured logging
- Atomic metrics (lock-free counters)
- Live monitoring dashboard with real-time charts
- Graceful shutdown (SIGINT/SIGTERM)
- Docker and systemd deployment support

---

## Project Structure

```
├── include/server/          # Header files
│   ├── tcp_server.hpp       # Core TCP server
│   ├── thread_pool.hpp      # Fixed-size thread pool
│   ├── blocking_queue.hpp   # Thread-safe task queue
│   ├── http_parser.hpp      # HTTP/1.1 parser
│   ├── router.hpp           # URL routing
│   ├── metrics.hpp          # Atomic metrics singleton
│   └── logger.hpp           # Thread-safe logger
├── src/                     # Implementation files
├── public/                  # Static files (served by server AND GitHub Pages)
│   ├── index.html           # Landing page
│   ├── dashboard.html       # Monitoring dashboard
│   ├── dashboard.js         # Dashboard logic
│   ├── dashboard.css        # Dashboard styling
│   └── style.css            # Landing page styling
├── tests/                   # 41 unit tests
├── deploy/                  # systemd service file
├── Dockerfile               # Docker build
└── docs/                    # Architecture & deployment docs
```

---

## Testing

```bash
./build/tests/server_tests
```

41 tests covering: HTTP parser, router, thread pool, blocking queue, static file serving.

---

## Technologies

| Category | Implementation |
|----------|---------------|
| Language | C++17 |
| Networking | Raw TCP sockets |
| Concurrency | std::thread, std::mutex, std::condition_variable |
| Metrics | std::atomic counters |
| Build | CMake + Ninja |
| Frontend | HTML, CSS, Vanilla JavaScript |
| Deployment | Docker, systemd, GitHub Pages, ngrok |

---

## Limitations

- Educational HTTP server, not a production replacement for Nginx/Apache
- No HTTP/1.1 keep-alive (Connection: close)
- No TLS/HTTPS (use a reverse proxy)
- No chunked transfer encoding
- Synchronous I/O model
- No URL query string parsing

---

## License

MIT
