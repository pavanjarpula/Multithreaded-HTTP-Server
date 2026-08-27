# Architecture

## System Overview

The server follows a single-acceptor, thread-pool architecture:

```
┌──────────────────────────────────────────────────────┐
│                    Main Thread                        │
│  ┌─────────────┐                                     │
│  │ Signal Handler│─── SIGINT/SIGTERM ──→ stop()      │
│  └─────────────┘                                     │
│  ┌─────────────┐     ┌────────────────────────────┐  │
│  │ accept_loop  │────→│ Task Queue (BlockingQueue) │  │
│  └─────────────┘     └──────────┬─────────────────┘  │
└──────────────────────────────────┼────────────────────┘
                                   │
                    ┌──────────────┼──────────────┐
                    │              │              │
              ┌─────▼─────┐ ┌─────▼─────┐ ┌─────▼─────┐
              │  Worker 1  │ │  Worker 2  │ │  Worker N  │
              └─────┬──────┘ └─────┬──────┘ └─────┬──────┘
                    │              │              │
              ┌─────▼──────────────▼──────────────▼─────┐
              │              Request Handler             │
              │  ┌──────────┐  ┌────────┐  ┌─────────┐  │
              │  │HttpParser│→ │ Router │→ │Response │  │
              │  └──────────┘  └───┬────┘  └─────────┘  │
              │                    │                     │
              │         ┌─────────┼─────────┐           │
              │         │         │         │           │
              │    ┌────▼───┐ ┌───▼────┐ ┌──▼──────┐  │
              │    │Dynamic │ │Static  │ │Metrics  │  │
              │    │Routes  │ │Files   │ │Logger   │  │
              │    └────────┘ └────────┘ └─────────┘  │
              └────────────────────────────────────────┘
```

## Component Descriptions

### Socket Layer (`socket_compat.hpp`)
Cross-platform socket abstraction providing:
- `socket_t` type alias (`SOCKET` on Windows, `int` on POSIX)
- `close_socket()` — platform-agnostic close
- `last_socket_error()` — get last error code
- `invalid_socket()` — platform-appropriate invalid sentinel

### TCP Server (`tcp_server.hpp/cpp`)
Core networking component:
- Creates listening socket with `SO_REUSEADDR`
- `accept_loop()` runs in the main thread
- Submits client fds to the thread pool
- `handle_client()` runs in worker threads
- Handles partial reads/writes, timeouts, error recovery

### Thread Pool (`thread_pool.hpp`)
Fixed-size reusable thread pool:
- `BlockingQueue<std::function<void()>>` for task dispatch
- Workers block on condition variable when idle
- `submit()` for non-blocking task enqueue
- `shutdown()` stops accepting, drains queue, joins threads
- Exception-safe worker boundaries

### Blocking Queue (`blocking_queue.hpp`)
Thread-safe producer/consumer queue:
- `std::mutex` for synchronization
- `std::condition_variable` for blocking wait
- `wait_and_pop()` — blocking with shutdown support
- `try_pop()` — non-blocking attempt
- `shutdown()` — signals all waiters

### HTTP Parser (`http_parser.hpp/cpp`)
HTTP/1.1 request parser:
- Parses request line, headers, body
- Content-Length body extraction
- Returns `ParseResult` with error codes
- Limits: 64KB headers, 1MB body

### Router (`router.hpp/cpp`)
Request dispatching:
- `std::unordered_map<path, unordered_map<method, handler>>`
- Method/path matching with 405 support
- Delegates unmatched GET/HEAD to static file handler
- Built-in routes: /, /health, /hello, /metrics, /echo

### Static File Handler (`static_file_handler.hpp/cpp`)
Secure file serving:
- `std::filesystem` for path resolution and validation
- MIME type detection from extension
- URL decoding (%xx, +)
- Path normalization (resolve . and ..)
- Security: resolved path must stay within document root

### Logger (`logger.hpp`)
Thread-safe structured logging:
- `std::mutex` prevents interleaved output
- Timestamp, level, thread ID
- Output to stdout and optional file

### Metrics (`metrics.hpp`)
Thread-safe performance tracking:
- `std::atomic` counters for hot-path metrics
- `std::mutex` for latency tracking (atomic<double> lacks fetch_add)
- JSON serialization for `/metrics` endpoint
- Tracks: requests, errors, connections, bytes, latency, uptime

## Data Flow

1. Client connects → `accept()` returns fd
2. fd submitted to thread pool queue
3. Worker dequeues and calls `handle_client(fd)`
4. `recv_request()` reads until `\r\n\r\n` and Content-Length body
5. `HttpParser::parse()` extracts structured request
6. `Router::handle()` dispatches to handler
7. Handler returns `HttpResponse`
8. `serialize()` builds HTTP response string
9. `send_all()` writes complete response
10. Connection closed, metrics updated
