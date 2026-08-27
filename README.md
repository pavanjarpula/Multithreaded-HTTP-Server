# C++ Multithreaded HTTP/1.1 Web Server

A production-quality educational HTTP/1.1 web server built from scratch using low-level TCP sockets, C++17, and standard threading primitives. No web frameworks (Boost.Beast, Crow, Drogon, Pistache) used.

---

## Features

| Category | Implementation |
|----------|---------------|
| **TCP Sockets** | Raw `socket()`, `bind()`, `listen()`, `accept()`, `recv()`, `send()` with cross-platform abstraction |
| **HTTP/1.1** | Request line parsing, headers, Content-Length body, method validation |
| **Threading** | Fixed-size thread pool with blocking task queue, condition variables, no busy-wait |
| **Routing** | Method/path matching, 404/405 responses with Allow headers |
| **Static Files** | MIME type detection, directory traversal prevention, path normalization |
| **Logging** | Thread-safe structured logging with timestamp, level, thread ID, latency |
| **Metrics** | Atomic counters for requests, errors, connections, bytes, latency, uptime |
| **Shutdown** | Graceful SIGINT/SIGTERM handling, thread pool drain, socket cleanup |
| **Platform** | Winsock2 (Windows) and POSIX sockets (Linux/macOS) |

---

## Architecture

```
                         ┌──────────────────────┐
                         │     Client (curl)    │
                         └──────────┬───────────┘
                                    │ TCP connection
                         ┌──────────▼───────────┐
                         │   Main Thread        │
                         │   accept() loop      │
                         └──────────┬───────────┘
                                    │ submit task
                         ┌──────────▼───────────┐
                         │   BlockingQueue      │
                         │   (thread-safe)      │
                         └──────────┬───────────┘
                                    │ dequeue
                    ┌───────────────┼───────────────┐
                    │               │               │
              ┌─────▼─────┐  ┌─────▼─────┐  ┌─────▼─────┐
              │  Worker 1  │  │  Worker 2  │  │  Worker N  │
              │  std::thread│  │  std::thread│  │  std::thread│
              └─────┬──────┘  └─────┬──────┘  └─────┬──────┘
                    │               │               │
                    └───────────────┼───────────────┘
                                    │
                    ┌───────────────▼───────────────┐
                    │         Request Handler       │
                    │                               │
                    │  1. recv_request()             │
                    │  2. HttpParser::parse()        │
                    │  3. Router::handle()           │
                    │     ├── Dynamic route handler  │
                    │     └── StaticFileHandler      │
                    │  4. HttpResponse::serialize()  │
                    │  5. send_all()                 │
                    └───────────────────────────────┘
```

---

## Build Instructions

### Prerequisites

- **C++17 compiler**: GCC 7+, Clang 5+, or MSVC 2017+
- **CMake**: 3.16 or higher
- **Ninja** (recommended) or Make

### Build Commands

```bash
# Configure (Debug)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# Configure (Release)
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build Release
cmake --build build-release
```

On Windows with MinGW:
```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

## Running the Server

### PowerShell (Windows)

```powershell
# Start the server
cd "C:\Desktop\Github Projects\Multithreaded HTTP Server"
.\build\http_server.exe --port 8080 --threads 4

# In a NEW PowerShell window, test with curl.exe (not curl, which is an alias)
curl.exe http://localhost:8080/hello
```

### Linux / macOS

```bash
./build/http_server --port 8080 --threads 8
```

### Command-Line Options

| Flag | Default | Description |
|------|---------|-------------|
| `--port PORT` | `8080` | TCP port to listen on |
| `--threads N` | Hardware concurrency | Number of worker threads |
| `--root PATH` | `public` | Document root for static files |
| `--help`, `-h` | | Show usage information |

---

## API Endpoints

### GET `/` — Welcome Page

```powershell
curl.exe http://localhost:8080/
```

Returns an HTML welcome page listing available endpoints.

---

### GET `/health` — Health Check

```powershell
curl.exe http://localhost:8080/health
```

**Response (200 OK):**
```json
{"status":"ok"}
```

---

### GET `/hello` — Hello World

```powershell
curl.exe http://localhost:8080/hello
```

**Response (200 OK):**
```
Hello, World!
```

---

### POST `/echo` — Echo Request Body

```powershell
curl.exe -X POST -d "hello world" http://localhost:8080/echo
```

**Response (200 OK):**
```
hello world
```

---

### GET `/metrics` — Server Metrics

```powershell
curl.exe http://localhost:8080/metrics
```

**Response (200 OK):**
```json
{
  "total_requests": 12,
  "successful_requests": 10,
  "client_errors": 1,
  "server_errors": 0,
  "active_connections": 1,
  "peak_connections": 3,
  "total_bytes_sent": 4096,
  "average_latency_ms": 0.45,
  "uptime_seconds": 60
}
```

**Metrics Fields:**

| Field | Type | Description |
|-------|------|-------------|
| `total_requests` | uint64 | Total requests received since startup |
| `successful_requests` | uint64 | Requests that returned 2xx status |
| `client_errors` | uint64 | Requests that returned 4xx status |
| `server_errors` | uint64 | Requests that returned 5xx status |
| `active_connections` | int64 | Currently open connections |
| `peak_connections` | uint64 | Maximum concurrent connections observed |
| `total_bytes_sent` | uint64 | Total bytes sent to clients |
| `average_latency_ms` | double | Average request processing time in milliseconds |
| `uptime_seconds` | int64 | Server uptime since startup |

---

### Static Files

```powershell
curl.exe http://localhost:8080/style.css
curl.exe http://localhost:8080/hello.txt
```

**Supported MIME Types:**

| Extension | Content-Type |
|-----------|-------------|
| `.html`, `.htm` | `text/html` |
| `.css` | `text/css` |
| `.js` | `application/javascript` |
| `.json` | `application/json` |
| `.txt` | `text/plain` |
| `.png` | `image/png` |
| `.jpg`, `.jpeg` | `image/jpeg` |
| `.svg` | `image/svg+xml` |
| `.ico` | `image/x-icon` |
| `.gif` | `image/gif` |
| `.woff`, `.woff2` | `font/woff`, `font/woff2` |
| `.ttf` | `font/ttf` |
| `.pdf` | `application/pdf` |
| `.xml` | `application/xml` |

---

## HTTP Status Codes

| Code | Meaning | When Returned |
|------|---------|---------------|
| 200 | OK | Successful request |
| 400 | Bad Request | Malformed HTTP request, incomplete headers |
| 403 | Forbidden | Directory traversal attempt, path escape |
| 404 | Not Found | No matching route or static file |
| 405 | Method Not Allowed | Valid path but wrong HTTP method |
| 413 | Payload Too Large | Headers > 64KB or body > 1MB |
| 500 | Internal Server Error | Unhandled exception in request handler |
| 501 | Not Implemented | Registered but unimplemented handler |

---

## Concurrency Model

```
Main Thread (accept loop)
    │
    ├── Creates listening socket (SO_REUSEADDR)
    ├── Binds to port, starts listening
    ├── Creates ThreadPool(N workers)
    ├── accept() loop:
    │     ├── Accept connection → get client fd
    │     ├── pool.submit(handle_client(fd))
    │     └── Loop back to accept()
    │
    ├── Signal handler sets running_ = false
    ├── Closes listening socket (wakes blocked accept)
    └── pool.shutdown() → joins all workers

Worker Threads (N = --threads)
    │
    ├── Loop:
    │     ├── queue.wait_and_pop()  [blocks on condition_variable]
    │     ├── Execute task: handle_client(fd)
    │     │     ├── recv_request(fd)         → read HTTP request
    │     │     ├── HttpParser::parse()      → parse request
    │     │     ├── Router::handle()         → dispatch to handler
    │     │     ├── HttpResponse::serialize() → build response
    │     │     ├── send_all(fd, response)   → send complete response
    │     │     └── close_socket(fd)         → close connection
    │     └── Loop back to wait_and_pop()
    │
    └── On shutdown: exit loop, thread joins
```

**Synchronization primitives used:**
- `std::atomic<bool>` — shutdown flag, stop signal
- `std::mutex` + `std::condition_variable` — blocking queue (workers sleep when idle)
- `std::atomic<uint64_t>` — metrics counters (lock-free)
- `std::mutex` — log output, latency tracking

---

## Request Lifecycle

```
1. Client connects via TCP
2. Main thread: accept() returns client_fd
3. Task submitted to BlockingQueue
4. Worker thread picks up task
5. recv_request(): reads until "\r\n\r\n" + Content-Length body
6. HttpParser::parse(): extracts method, path, headers, body
7. Router::handle(): matches route → calls handler function
8. Handler returns HttpResponse with status, headers, body
9. HttpResponse::serialize(): builds "HTTP/1.1 200 OK\r\n..."
10. send_all(): writes complete response to socket
11. close_socket(): closes connection
12. Metrics updated (requests, bytes, latency)
13. Log entry written: "GET /hello -> 200 (0.4 ms) [127.0.0.1:54321]"
```

---

## Testing

### Run All Tests

```bash
cmake --build build
./build/tests/server_tests
```

**Test Results:**
```
=== C++ HTTP Server Test Suite ===
40/40 tests passed
```

### Test Breakdown

| Test Suite | Tests | What's Tested |
|-----------|-------|---------------|
| **HTTP Parser** | 10 | Valid GET/POST/HEAD, headers, body parsing, malformed requests, incomplete requests, unknown methods |
| **Router** | 8 | Route matching, 404 not found, 405 method not allowed, static file delegation, HEAD support |
| **Thread Pool** | 8 | Task execution, concurrent work, FIFO ordering, shutdown behavior, multiple producers, exception safety |
| **Static Files** | 14 | File serving, missing files, directory traversal blocking, URL decoding, MIME types (html, css, js, json, txt, png, unknown) |

---

## Benchmarking

### Using Apache Bench (ab)

```bash
# Start the server
./build/http_server --port 8080 --threads 8

# Low concurrency (10 simultaneous connections)
ab -n 10000 -c 10 http://127.0.0.1:8080/hello

# Medium concurrency (50 simultaneous connections)
ab -n 10000 -c 50 http://127.0.0.1:8080/hello

# High concurrency (100 simultaneous connections)
ab -n 10000 -c 100 http://127.0.0.1:8080/hello

# Static file benchmark
ab -n 10000 -c 50 http://127.0.0.1:8080/style.css

# POST echo benchmark
ab -n 10000 -c 50 -p post_body.txt -T text/plain http://127.0.0.1:8080/echo
```

### Using wrk

```bash
# 4 threads, 100 connections, 30 seconds
wrk -t4 -c100 -d30s http://127.0.0.1:8080/hello

# Higher load
wrk -t8 -c200 -d30s http://127.0.0.1:8080/hello
```

### Benchmark Script

```bash
bash benchmarks/benchmark.sh 8080 8
```

### Metrics to Collect

| Metric | Description |
|--------|-------------|
| Requests/sec | Total requests completed per second |
| Average latency | Mean response time |
| p50 latency | Median response time |
| p99 latency | 99th percentile (tail latency) |
| Transfer rate | KB/sec throughput |
| Failed requests | Connection errors, timeouts |

---

## Project Structure

```
├── CMakeLists.txt                  # Build configuration (C++17, warnings, platform linking)
├── README.md                       # This file
├── .gitignore
│
├── include/server/                 # Header files
│   ├── config.hpp                  # Config struct, CLI argument parsing
│   ├── logger.hpp                  # Thread-safe Logger singleton
│   ├── metrics.hpp                 # Thread-safe Metrics singleton (atomics + mutex)
│   ├── socket_compat.hpp           # Cross-platform socket types (Winsock2/POSIX)
│   ├── blocking_queue.hpp          # Thread-safe BlockingQueue<T>
│   ├── thread_pool.hpp             # Fixed-size ThreadPool
│   ├── tcp_server.hpp              # TcpServer class declaration
│   ├── http_request.hpp            # HttpRequest struct, HttpMethod enum
│   ├── http_response.hpp           # HttpResponse class, StatusCode enum
│   ├── http_parser.hpp             # HttpParser class declaration
│   ├── router.hpp                  # Router class declaration
│   └── static_file_handler.hpp     # StaticFileHandler class declaration
│
├── src/                            # Implementation files
│   ├── main.cpp                    # Entry point, signal handlers, Winsock init
│   ├── tcp_server.cpp              # Socket bind/listen/accept, request handling
│   ├── http_parser.cpp             # HTTP/1.1 request parser
│   ├── http_response.cpp           # (Inline in header)
│   ├── router.cpp                  # Route definitions, dispatching logic
│   ├── static_file_handler.cpp     # File serving, path security, MIME types
│   ├── logger.cpp                  # (Inline in header)
│   └── metrics.cpp                 # (Inline in header)
│
├── public/                         # Static files served by the server
│   ├── index.html                  # Welcome page
│   ├── style.css                   # CSS stylesheet
│   └── hello.txt                   # Plain text file
│
├── tests/                          # Unit tests (40 test cases)
│   ├── CMakeLists.txt              # Test build configuration
│   ├── test_framework.hpp          # Lightweight test framework (TEST, ASSERT_*)
│   ├── test_main.cpp               # Test runner entry point
│   ├── test_http_parser.cpp        # 10 parser tests
│   ├── test_router.cpp             # 8 router tests
│   ├── test_thread_pool.cpp        # 8 thread pool tests
│   └── test_static_files.cpp       # 14 static file tests
│
├── benchmarks/
│   └── benchmark.sh                # Apache Bench benchmarking script
│
└── docs/
    ├── architecture.md             # Detailed architecture with diagrams
    ├── design-decisions.md         # Design rationale and trade-offs
    └── benchmarking.md             # Benchmarking methodology and tools
```

---

## Design Decisions

| Decision | Rationale |
|----------|-----------|
| **Thread pool** over thread-per-connection | Bounds resource usage, avoids thread explosion, better cache locality |
| **Atomic metrics** over mutex-protected | Hot-path counters avoid lock contention entirely |
| **Connection: close** over keep-alive | Simplicity and correctness; keep-alive adds timeout and state management complexity |
| **std::filesystem** for path validation | Cross-platform, built-in canonical resolution, prevents traversal attacks |
| **std::optional** for parser results | Clean error signaling without exceptions for expected failure cases |
| **Mutex for latency tracking** | `std::atomic<double>` lacks `fetch_add` in C++17 |

---

## Limitations

- No HTTP/1.1 keep-alive (connections close after each request)
- No chunked transfer encoding
- No TLS/HTTPS support
- No WebSocket support
- No URL query string parsing (`?key=value`)
- No request body streaming (full body buffered before processing)
- File serving uses synchronous I/O
- No URL path parameters (`/users/:id`)

## Future Improvements

- HTTP/1.1 keep-alive with idle timeouts and max requests per connection
- `epoll` / `kqueue` / `io_uring` for high-performance I/O multiplexing
- Request body streaming for large uploads
- URL query parameter parsing
- TLS support via OpenSSL
- HTTP/2 multiplexing
- JSON configuration file
- Prometheus-compatible metrics endpoint (`/metrics` with proper formatting)
- Access logging to file
- Rate limiting per IP
- Request timeout enforcement
