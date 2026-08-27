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
    explicit ThreadPool(unsigned int num_threads = std::thread::hardware_concurrency()) {
        if (num_threads == 0) num_threads = 2;

        workers_.reserve(num_threads);
        for (unsigned int i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] { worker_loop(); });
        }
    }

    void submit(std::function<void()> task) {
        if (stopped_.load(std::memory_order_relaxed)) {
            throw std::runtime_error("Cannot submit task to stopped thread pool");
        }
        queue_.push(std::move(task));
    }

    void shutdown() {
        bool expected = false;
        if (!stopped_.compare_exchange_strong(expected, true, std::memory_order_acquire)) {
            return;
        }

        queue_.shutdown();

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

    std::size_t queue_size() const { return queue_.size(); }
    std::size_t thread_count() const { return workers_.size(); }

private:
    void worker_loop() {
        while (true) {
            auto task_opt = queue_.wait_and_pop();
            if (!task_opt.has_value()) {
                break;
            }

            try {
                task_opt.value()();
            } catch (const std::exception& e) {
                LOG_ERROR("Worker thread exception: " + std::string(e.what()));
            } catch (...) {
                LOG_ERROR("Worker thread caught unknown exception");
            }
        }
    }

    std::vector<std::thread> workers_;
    BlockingQueue<std::function<void()>> queue_;
    std::atomic<bool> stopped_{false};
};

} // namespace server
