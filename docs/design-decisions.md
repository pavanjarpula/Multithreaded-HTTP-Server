# Design Decisions

## Thread Pool vs Thread-per-Connection

**Decision:** Fixed-size thread pool with task queue.

**Rationale:**
- Thread-per-connection creates unbounded threads under load, exhausting memory and causing context-switch overhead
- Thread pool bounds resource usage to a fixed number of workers
- Reusable threads avoid thread creation/destruction overhead per request
- Task queue provides natural backpressure when all workers are busy
- Worker threads block on condition variable when idle, consuming no CPU

**Trade-off:**
- Under extreme load, requests queue up rather than being rejected immediately
- Pool size must be tuned for the workload (CPU-bound vs I/O-bound)

## Atomic Counters vs Mutex-Protected Metrics

**Decision:** `std::atomic` for hot-path counters, `std::mutex` for latency.

**Rationale:**
- Metrics counters (requests, bytes) are updated on every request — atomic avoids lock contention
- `std::atomic::fetch_add` compiles to a single `lock xadd` instruction on x86
- Latency uses `std::mutex` because `std::atomic<double>` lacks `fetch_add` in C++17
- `compare_exchange_weak` in peak connection tracking avoids ABA problems

## Connection: Close vs Keep-Alive

**Decision:** `Connection: close` (no keep-alive).

**Rationale:**
- Keep-alive requires tracking idle timeouts, max requests per connection, partial request buffering
- Close-after-response is simpler, correct, and sufficient for the educational scope
- Each request gets a fresh connection, avoiding state management bugs

## std::filesystem vs Platform APIs

**Decision:** `std::filesystem` for file operations.

**Rationale:**
- Cross-platform path resolution, canonicalization, file type checking
- `std::filesystem::canonical()` prevents symlink attacks
- Modern C++17 idiomatic approach
- Falls back to `GetFullPathNameA()` on older Windows if needed

## Single-Acceptor Design

**Decision:** Main thread runs accept loop; workers handle requests.

**Rationale:**
- Single acceptor avoids thundering herd (multiple threads competing for accept)
- `accept()` is fast compared to request processing
- On Linux, `epoll` could enable multiple acceptors, but adds complexity
- For educational purposes, simplicity is preferred

## HTTP Request Parser Design

**Decision:** Parse from complete buffered request (not streaming).

**Rationale:**
- `recv_request()` buffers the entire request before parsing
- Simpler than incremental parsing state machines
- Sufficient for the 1MB body limit
- Content-Length provides exact body boundary
- Trade-off: higher memory per connection, but bounded by max_body_size

## Logging Architecture

**Decision:** Mutex-protected logger with structured output.

**Rationale:**
- `std::mutex` prevents interleaved output from concurrent threads
- Structured format (timestamp, level, thread ID) aids debugging
- Includes request latency for performance analysis
- `fflush(stdout)` ensures immediate visibility

## Error Handling Strategy

**Decision:** Exceptions at boundaries, error codes internally.

**Rationale:**
- Fatal errors (socket creation, bind) throw to abort startup
- Worker threads catch all exceptions to prevent thread death
- HTTP errors return status codes rather than exceptions
- Parser returns error enum for clear error classification
