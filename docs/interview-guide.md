# Interview Guide

## Project Overview
A production-grade C++17 multithreaded HTTP/1.1 server demonstrating core systems programming concepts: socket programming, concurrency, memory management, and real-time monitoring.

## Architecture Deep Dive

### 1. Request Lifecycle
```
Client Request
  → TCP accept() on main thread
  → Socket FD + client info copied
  → Submitted to BoundedBlockingQueue (HTTP 503 if full)
  → Worker thread wakes via condition_variable.wait()
  → recv() reads raw HTTP bytes
  → HttpParser::parse() extracts method, path, headers, body
  → Router matches path → Handler generates response
  → HttpResponse::serialize() builds raw HTTP response
  → send_all() writes to socket
  → close_socket() + metrics updated
```

### 2. Thread Pool Design
- **Fixed-size pool**: `N` worker threads created at startup, never recreated
- **Bounded blocking queue**: Capacity-limited (default 50). Push returns false when full → HTTP 503
- **Worker loop**: `wait_and_pop()` blocks on `condition_variable::wait()`. When task arrives, `notify_one()` wakes exactly one worker
- **Active worker tracking**: Atomic counter incremented on task pickup, decremented on completion
- **Graceful shutdown**: `shutdown()` sets flag, notifies all, joins all threads

### 3. Bounded Queue & Backpressure
```cpp
bool push(T item) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.size() >= capacity_) return false;  // backpressure!
    queue_.push(std::move(item));
    cv_.notify_one();
    return true;
}
```
- Prevents unbounded memory growth under load
- Returns HTTP 503 with `Retry-After: 5` header
- Metrics expose `queue_size`, `queue_capacity` for monitoring

### 4. Synchronization Strategy
| Mechanism | Usage |
|-----------|-------|
| `std::mutex` | Queue access, latency accumulator |
| `std::condition_variable` | Worker wake-up on task submission |
| `std::atomic<uint64_t>` | All metrics counters (lock-free) |
| `std::atomic<bool>` | Server running flag, pool stopped flag |

### 5. HTTP Parser
- Hand-written state machine, zero-copy where possible
- Handles: `GET`, `POST`, `PUT`, `DELETE`, `HEAD`
- Validates: Content-Length, header size limits, body size limits
- Returns structured `ParseResult` with error enum

## Key Talking Points

### Why Thread Pool over Per-Connection Thread?
- Thread creation/destruction has ~10-50μs overhead
- Fixed pool amortizes startup cost
- Bounds memory usage (max N concurrent requests)
- OS scheduler has predictable workload

### Why Bounded Queue?
- Unbounded queue = unbounded memory under load
- Bounded queue + 503 = graceful degradation
- Client gets explicit "server busy" instead of OOM kill
- Queue metrics enable capacity planning

### Lock-Free Metrics
- `std::atomic::fetch_add` compiles to single `LOCK XADD` on x86
- No mutex contention on hot path
- Trade-off: slightly stale reads (acceptable for monitoring)

## Common Interview Questions

**Q: How does this scale beyond 4 threads?**
A: The thread count is configurable via `--threads`. On an 8-core machine, set to 8. Beyond CPU cores, context switching overhead dominates. For higher concurrency, you'd need epoll/kqueue (not in scope).

**Q: What happens when the queue is full?**
A: `pool_->submit()` returns false. The accept loop immediately sends HTTP 503 `Service Unavailable` with `Retry-After: 5` and closes the connection. No task is queued.

**Q: How do you handle partial HTTP requests?**
A: `recv_request()` loops until it sees `\r\n\r\n`. Then it parses Content-Length and reads remaining body bytes. Timeout (10s) prevents hanging connections.

**Q: What's the memory footprint per connection?**
A: Socket FD (small) + request buffer (~4KB initial) + stack for worker thread (~8MB default, shared). Under 10KB per idle connection.

**Q: How would you add keep-alive?**
A: After sending response, check `Connection: keep-alive` header. If present, loop back to `recv_request()` instead of closing socket. Add idle timeout.

**Q: How would you add TLS?**
A: Integrate OpenSSL: wrap `accept()` with `SSL_accept()`, wrap `recv/send` with `SSL_read/SSL_write`. Certificate loaded from file. Minimal changes to existing architecture.

## Running the Demo
```bash
# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Run with 4 threads, queue capacity 10
./http_server --threads 4 --capacity 10 --port 8080

# Test backpressure (fire 200 concurrent requests)
ab -n 200 -c 200 http://localhost:8080/work?delay=500

# Watch dashboard
open http://localhost:8080/dashboard

# Interactive playground
open http://localhost:8080/playground

# Concurrency lab
open http://localhost:8080/lab
```

## Metrics Reference
| Metric | Type | Description |
|--------|------|-------------|
| `total_requests` | counter | All received requests |
| `successful_requests` | counter | 2xx responses |
| `client_errors` | counter | 4xx responses |
| `server_errors` | counter | 5xx responses (includes 503 backpressure) |
| `active_connections` | gauge | Currently open connections |
| `peak_connections` | gauge | Historical maximum concurrent connections |
| `total_bytes_sent` | counter | Total bytes written to sockets |
| `average_latency_ms` | gauge | Mean request processing time |
| `requests_per_second` | gauge | Total requests / uptime seconds |
| `uptime_seconds` | gauge | Time since server start |
| `worker_threads` | gauge | Thread pool size |
| `active_workers` | gauge | Workers currently processing |
| `idle_workers` | gauge | Workers waiting for tasks |
| `queue_size` | gauge | Tasks waiting in queue |
| `queue_capacity` | gauge | Maximum queue size |
