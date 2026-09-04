# Architecture

## Overview

A single-acceptor, fixed-size thread-pool HTTP server.

```
Client
  │ TCP
  ▼
Main Thread (accept loop)
  │ submit task
  ▼
BlockingQueue<function<void()>>
  │ dequeue
  ▼
Worker Thread Pool (N threads)
  │
  ├── recv_request()
  ├── HttpParser::parse()
  ├── Router::handle()
  │     ├── Dynamic route handler
  │     └── StaticFileHandler
  ├── HttpResponse::serialize()
  ├── send_all()
  └── close socket
```

## Request Lifecycle

1. Client connects via TCP
2. `accept()` returns client file descriptor
3. fd submitted to `BlockingQueue`
4. Worker thread dequeues and calls `handle_client(fd)`
5. `recv_request()` reads until `\r\n\r\n` + Content-Length body
6. `HttpParser::parse()` extracts method, path, headers, body
7. `Router::handle()` dispatches to matching handler
8. Handler returns `HttpResponse`
9. `serialize()` builds HTTP/1.1 response string
10. `send_all()` writes complete response to socket
11. Socket closed, metrics updated

## Why a Thread Pool?

Thread-per-connection creates unbounded threads under load. A thread pool bounds resource usage:

```
Thread Per Connection         Thread Pool
─────────────────────         ───────────
1000 connections              1000 connections
= 1000 threads                = 8 worker threads
= OS scheduling overhead      = bounded memory
= thread explosion risk       = predictable latency
```

Trade-off: blocking I/O in worker threads. Acceptable for an educational server.

## Why a Blocking Queue?

Decouples accept rate from processing rate:

```
accept() ──push──▶ BlockingQueue ──pop──▶ Worker Thread
```

- Workers sleep on `condition_variable` when idle (zero CPU)
- `push()` wakes one worker via `notify_one()`
- `shutdown()` sets flag and calls `notify_all()` to wake all waiters

## Why Atomic Metrics?

Hot-path counters (total_requests, bytes_sent) update on every request. Using `std::atomic<uint64_t>` avoids mutex contention:

```
Worker Thread 1 ──fetch_add──▶ total_requests_ (atomic)
Worker Thread 2 ──fetch_add──▶ total_requests_ (atomic)
Worker Thread 3 ──fetch_add──▶ total_requests_ (atomic)
```

Latency uses mutex because `std::atomic<double>` lacks `fetch_add` in C++17.

## Components

| Component | File | Purpose |
|-----------|------|---------|
| TcpServer | `tcp_server.hpp/cpp` | Socket bind/listen/accept, client handling |
| ThreadPool | `thread_pool.hpp` | Fixed-size worker pool with task queue |
| BlockingQueue | `blocking_queue.hpp` | Thread-safe producer/consumer queue |
| HttpParser | `http_parser.hpp/cpp` | HTTP/1.1 request parsing |
| Router | `router.hpp/cpp` | URL dispatch, method matching |
| StaticFileHandler | `static_file_handler.hpp/cpp` | Secure file serving with MIME detection |
| Metrics | `metrics.hpp` | Atomic counters, JSON serialization |
| Logger | `logger.hpp` | Thread-safe structured logging |
