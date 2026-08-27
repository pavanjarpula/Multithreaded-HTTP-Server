#!/bin/bash
# Benchmarking script using Apache Bench (ab)
# Usage: bash benchmark.sh [port] [threads]

PORT=${1:-8080}
THREADS=${2:-8}
SERVER="./build/http_server"
URL="http://127.0.0.1:${PORT}"

echo "=== C++ HTTP Server Benchmark Suite ==="
echo "Port: ${PORT}, Threads: ${THREADS}"
echo ""

# Start server in background
${SERVER} --port ${PORT} --threads ${THREADS} &
SERVER_PID=$!
sleep 1

echo "--- Benchmark 1: /hello (10K requests, 10 concurrent) ---"
ab -n 10000 -c 10 ${URL}/hello
echo ""

echo "--- Benchmark 2: /hello (10K requests, 50 concurrent) ---"
ab -n 10000 -c 50 ${URL}/hello
echo ""

echo "--- Benchmark 3: /hello (10K requests, 100 concurrent) ---"
ab -n 10000 -c 100 ${URL}/hello
echo ""

echo "--- Benchmark 4: /health (10K requests, 50 concurrent) ---"
ab -n 10000 -c 50 ${URL}/health
echo ""

echo "--- Benchmark 5: /style.css (10K requests, 50 concurrent) ---"
ab -n 10000 -c 50 ${URL}/style.css
echo ""

# Stop server
kill ${SERVER_PID} 2>/dev/null
wait ${SERVER_PID} 2>/dev/null

echo "=== Benchmarks Complete ==="
