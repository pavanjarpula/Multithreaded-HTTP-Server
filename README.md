# C++ Concurrent HTTP Server

A multithreaded HTTP/1.1 server built from scratch using C++17 and low-level TCP sockets.

> Modern web frameworks hide the complexity of networking, HTTP processing, and concurrency. This project explores those underlying mechanisms by implementing a fully functional HTTP server from first principles.

---

## Live Demo

| | URL |
|---|---|
| Server | `http://your-server:8080` |
| Dashboard | `http://your-server:8080/dashboard` |
| Metrics API | `http://your-server:8080/metrics` |

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

## Quick Start

### Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Run

```bash
./build/http_server --port 8080 --threads 4
```

### Test

```bash
./build/tests/server_tests
```

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

## Dashboard

The dashboard at `/dashboard` displays real-time metrics fetched from `GET /metrics`:

- Request counts (total, success, client errors, server errors)
- Connection metrics (active, peak)
- Performance (average latency, requests/sec)
- Server info (uptime, threads, queue size)
- Live charts (requests/sec and latency over 60 seconds)

```
Browser → GET /metrics → C++ Server → Atomic Counters → JSON → Dashboard
```

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
├── public/                  # Static files (dashboard, CSS, JS)
├── tests/                   # 40 unit tests
├── deploy/                  # systemd service file
├── Dockerfile               # Docker build
├── docker-compose.yml       # Docker Compose
└── docs/                    # Architecture & deployment docs
```

---

## Deployment

### Docker

```bash
docker build -t cpp-http-server .
docker run -d -p 8080:8080 cpp-http-server
```

### systemd (Linux)

```bash
sudo cp deploy/http-server.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl start http-server
sudo systemctl enable http-server
```

### Commands

```bash
sudo systemctl status http-server    # Check status
sudo systemctl restart http-server   # Restart
sudo systemctl stop http-server      # Stop
sudo journalctl -u http-server -f    # View logs
```

See [docs/deployment.md](docs/deployment.md) for full instructions including HTTPS via reverse proxy.

---

## Testing

```bash
./build/tests/server_tests
```

40 tests covering: HTTP parser, router, thread pool, blocking queue, static file serving.

---

## Limitations

- Educational HTTP server, not a production replacement for Nginx/Apache
- No HTTP/1.1 keep-alive (Connection: close)
- No TLS/HTTPS (use a reverse proxy)
- No chunked transfer encoding
- Synchronous I/O model
- No URL query string parsing

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
| Deployment | Docker, systemd |

---

## License

MIT
