# Benchmarking Guide

## Tools

- **Apache Bench (ab)** — Simple, widely available
- **wrk** — Higher performance, Lua scripting
- **hey** — Modern alternative to ab

## Running Benchmarks

### Prerequisites

Start the server:
```bash
./build/http_server --port 8080 --threads 8
```

### Apache Bench (ab)

```bash
# Low concurrency
ab -n 10000 -c 10 http://127.0.0.1:8080/hello

# Medium concurrency
ab -n 10000 -c 50 http://127.0.0.1:8080/hello

# High concurrency
ab -n 10000 -c 100 http://127.0.0.1:8080/hello

# Static file
ab -n 10000 -c 50 http://127.0.0.1:8080/style.css

# POST echo
ab -n 10000 -c 50 -p echo_body.txt -T text/plain http://127.0.0.1:8080/echo
```

### wrk

```bash
# 4 threads, 100 connections, 30 seconds
wrk -t4 -c100 -d30s http://127.0.0.1:8080/hello

# Higher load
wrk -t8 -c200 -d30s http://127.0.0.1:8080/hello
```

### hey

```bash
# 50 concurrent, 10000 requests
hey -n 10000 -c 50 http://127.0.0.1:8080/hello

# 200 concurrent
hey -n 10000 -c 200 http://127.0.0.1:8080/hello
```

## Metrics to Collect

| Metric | Description |
|--------|-------------|
| Requests/sec | Total requests completed per second |
| Average latency | Mean response time |
| p50 latency | Median response time |
| p99 latency | 99th percentile response time |
| Transfer rate | KB/sec throughput |
| Failed requests | Connection errors, timeouts |

## Interpreting Results

- **Requests/sec** indicates throughput capacity
- **Latency percentiles** reveal tail latency behavior
- **Transfer rate** shows network utilization
- **Failed requests** indicate saturation or resource exhaustion

## Tuning Parameters

| Parameter | Effect |
|-----------|--------|
| `--threads N` | More workers = higher concurrency, more context switches |
| `--port P` | Multiple instances on different ports for load distribution |
| OS backlog | `listen()` backlog affects connection queuing |

## Notes

- Run benchmarks on localhost to isolate server performance from network
- Use `-k` (keep-alive) with ab only if the server supports it
- Warm up the server with a few requests before benchmarking
- Run each benchmark 3+ times and report median results
- Never fabricate benchmark results
