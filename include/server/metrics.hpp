#pragma once

#include <string>
#include <cstdint>
#include <atomic>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <mutex>

namespace server {

class Metrics {
public:
    static Metrics& instance() {
        static Metrics metrics;
        return metrics;
    }

    void record_request() { total_requests_.fetch_add(1, std::memory_order_relaxed); }
    void record_success() { successful_requests_.fetch_add(1, std::memory_order_relaxed); }
    void record_client_error() { client_errors_.fetch_add(1, std::memory_order_relaxed); }
    void record_server_error() { server_errors_.fetch_add(1, std::memory_order_relaxed); }

    void add_bytes_sent(std::uint64_t bytes) {
        total_bytes_sent_.fetch_add(bytes, std::memory_order_relaxed);
    }

    void connection_opened() {
        std::int64_t current = active_connections_.fetch_add(1, std::memory_order_relaxed) + 1;
        std::uint64_t peak = peak_connections_.load(std::memory_order_relaxed);
        while (static_cast<std::uint64_t>(current) > peak) {
            if (peak_connections_.compare_exchange_weak(peak, static_cast<std::uint64_t>(current),
                                                        std::memory_order_relaxed)) {
                break;
            }
        }
    }

    void connection_closed() {
        active_connections_.fetch_sub(1, std::memory_order_relaxed);
    }

    void record_latency(double ms) {
        std::lock_guard<std::mutex> lock(latency_mutex_);
        total_latency_ms_ += ms;
        latency_count_ += 1;
    }

    void set_queue_size(std::size_t size) {
        queue_size_.store(static_cast<std::uint64_t>(size), std::memory_order_relaxed);
    }

    void set_queue_capacity(std::size_t cap) {
        queue_capacity_.store(static_cast<std::uint64_t>(cap), std::memory_order_relaxed);
    }

    void set_thread_count(std::size_t count) {
        thread_count_.store(static_cast<std::uint64_t>(count), std::memory_order_relaxed);
    }

    void set_active_workers(std::size_t count) {
        active_workers_.store(static_cast<std::uint64_t>(count), std::memory_order_relaxed);
    }

    std::string to_json() const {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        long long uptime_secs = std::chrono::duration_cast<std::chrono::seconds>(
            now - start_time_).count();

        double avg_latency = 0.0;
        double requests_per_sec = 0.0;
        {
            std::lock_guard<std::mutex> lock(latency_mutex_);
            if (latency_count_ > 0) {
                avg_latency = total_latency_ms_ / static_cast<double>(latency_count_);
            }
            if (uptime_secs > 0) {
                requests_per_sec = static_cast<double>(total_requests_.load()) / static_cast<double>(uptime_secs);
            }
        }

        std::uint64_t tc = thread_count_.load();
        std::uint64_t aw = active_workers_.load();
        std::uint64_t iw = tc > aw ? tc - aw : 0;

        std::ostringstream oss;
        oss << "{\n"
            << "  \"total_requests\": " << total_requests_.load() << ",\n"
            << "  \"successful_requests\": " << successful_requests_.load() << ",\n"
            << "  \"client_errors\": " << client_errors_.load() << ",\n"
            << "  \"server_errors\": " << server_errors_.load() << ",\n"
            << "  \"active_connections\": " << active_connections_.load() << ",\n"
            << "  \"peak_connections\": " << peak_connections_.load() << ",\n"
            << "  \"total_bytes_sent\": " << total_bytes_sent_.load() << ",\n"
            << "  \"average_latency_ms\": " << avg_latency << ",\n"
            << "  \"requests_per_second\": " << std::fixed << std::setprecision(2) << requests_per_sec << ",\n"
            << "  \"uptime_seconds\": " << uptime_secs << ",\n"
            << "  \"worker_threads\": " << tc << ",\n"
            << "  \"active_workers\": " << aw << ",\n"
            << "  \"idle_workers\": " << iw << ",\n"
            << "  \"queue_size\": " << queue_size_.load() << ",\n"
            << "  \"queue_capacity\": " << queue_capacity_.load() << "\n"
            << "}";
        return oss.str();
    }

    Metrics(const Metrics&) = delete;
    Metrics& operator=(const Metrics&) = delete;

private:
    Metrics()
        : start_time_(std::chrono::steady_clock::now())
        , total_requests_(0)
        , successful_requests_(0)
        , client_errors_(0)
        , server_errors_(0)
        , active_connections_(0)
        , peak_connections_(0)
        , total_bytes_sent_(0)
        , queue_size_(0)
        , queue_capacity_(0)
        , thread_count_(0)
        , active_workers_(0)
        , total_latency_ms_(0.0)
        , latency_count_(0) {
    }

    std::chrono::steady_clock::time_point start_time_;
    std::atomic<std::uint64_t> total_requests_;
    std::atomic<std::uint64_t> successful_requests_;
    std::atomic<std::uint64_t> client_errors_;
    std::atomic<std::uint64_t> server_errors_;
    std::atomic<std::int64_t> active_connections_;
    std::atomic<std::uint64_t> peak_connections_;
    std::atomic<std::uint64_t> total_bytes_sent_;
    std::atomic<std::uint64_t> queue_size_;
    std::atomic<std::uint64_t> queue_capacity_;
    std::atomic<std::uint64_t> thread_count_;
    std::atomic<std::uint64_t> active_workers_;

    mutable std::mutex latency_mutex_;
    double total_latency_ms_;
    std::uint64_t latency_count_;
};

} // namespace server
