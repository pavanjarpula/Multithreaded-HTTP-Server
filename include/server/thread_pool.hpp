#pragma once

#include "blocking_queue.hpp"
#include "logger.hpp"

#include <vector>
#include <thread>
#include <functional>
#include <atomic>
#include <stdexcept>

namespace server {

class ThreadPool {
public:
    explicit ThreadPool(unsigned int num_threads = std::thread::hardware_concurrency(),
                        std::size_t queue_capacity = 50) {
        if (num_threads == 0) num_threads = 2;

        queue_capacity_ = queue_capacity;
        queue_ = std::make_unique<BoundedBlockingQueue<std::function<void()>>>(queue_capacity);

        workers_.reserve(num_threads);
        for (unsigned int i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] { worker_loop(); });
        }
    }

    bool submit(std::function<void()> task) {
        if (stopped_.load(std::memory_order_relaxed)) {
            return false;
        }
        return queue_->push(std::move(task));
    }

    void shutdown() {
        bool expected = false;
        if (!stopped_.compare_exchange_strong(expected, true, std::memory_order_acquire)) {
            return;
        }

        queue_->shutdown();

        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    ~ThreadPool() {
        shutdown();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    std::size_t queue_size() const { return queue_ ? queue_->size() : 0; }
    std::size_t queue_capacity() const { return queue_capacity_; }
    std::size_t thread_count() const { return workers_.size(); }
    std::size_t active_workers() const { return active_workers_.load(std::memory_order_relaxed); }
    std::size_t idle_workers() const { return thread_count() - active_workers(); }
    bool is_queue_full() const { return queue_ ? queue_->is_full() : false; }

private:
    void worker_loop() {
        while (true) {
            auto task_opt = queue_->wait_and_pop();
            if (!task_opt.has_value()) {
                break;
            }

            active_workers_.fetch_add(1, std::memory_order_relaxed);

            try {
                task_opt.value()();
            } catch (const std::exception& e) {
                LOG_ERROR("Worker thread exception: " + std::string(e.what()));
            } catch (...) {
                LOG_ERROR("Worker thread caught unknown exception");
            }

            active_workers_.fetch_sub(1, std::memory_order_relaxed);
        }
    }

    std::vector<std::thread> workers_;
    std::unique_ptr<BoundedBlockingQueue<std::function<void()>>> queue_;
    std::size_t queue_capacity_ = 50;
    std::atomic<bool> stopped_{false};
    std::atomic<std::size_t> active_workers_{0};
};

} // namespace server
