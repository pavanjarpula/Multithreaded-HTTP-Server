#include "test_framework.hpp"
#include "server/blocking_queue.hpp"
#include "server/thread_pool.hpp"

#include <atomic>
#include <vector>
#include <chrono>
#include <thread>

using namespace server;

TEST(blocking_queue_push_pop) {
    BlockingQueue<int> q;
    q.push(42);
    auto val = q.try_pop();
    ASSERT_TRUE(val.has_value());
    ASSERT_EQ(val.value(), 42);
}

TEST(blocking_queue_empty_pop) {
    BlockingQueue<int> q;
    auto val = q.try_pop();
    ASSERT_FALSE(val.has_value());
}

TEST(blocking_queue_fifo) {
    BlockingQueue<int> q;
    q.push(1);
    q.push(2);
    q.push(3);
    ASSERT_EQ(q.try_pop().value(), 1);
    ASSERT_EQ(q.try_pop().value(), 2);
    ASSERT_EQ(q.try_pop().value(), 3);
}

TEST(blocking_queue_shutdown) {
    BlockingQueue<int> q;
    q.shutdown();
    auto val = q.wait_and_pop();
    ASSERT_FALSE(val.has_value());
}

TEST(blocking_queue_threaded) {
    BlockingQueue<int> q;
    std::atomic<int> sum{0};

    // Producer
    std::thread producer([&q]() {
        for (int i = 0; i < 100; ++i) {
            q.push(i);
        }
    });

    // Consumer
    std::thread consumer([&q, &sum]() {
        for (int i = 0; i < 100; ++i) {
            auto val = q.wait_and_pop();
            if (val.has_value()) {
                sum += val.value();
            }
        }
    });

    producer.join();
    consumer.join();

    // Sum of 0..99 = 4950
    ASSERT_EQ(sum.load(), 4950);
}

TEST(thread_pool_executes_tasks) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};

    for (int i = 0; i < 100; ++i) {
        pool.submit([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    // Wait for tasks to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    pool.shutdown();

    ASSERT_EQ(counter.load(), 100);
}

TEST(thread_pool_concurrent_work) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};
    const int num_tasks = 1000;

    for (int i = 0; i < num_tasks; ++i) {
        pool.submit([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }

    pool.shutdown();
    ASSERT_EQ(counter.load(), num_tasks);
}

TEST(thread_pool_shutdown_prevents_new_tasks) {
    ThreadPool pool(2);
    pool.shutdown();

    bool threw = false;
    try {
        pool.submit([]() {});
    } catch (const std::runtime_error&) {
        threw = true;
    }
    ASSERT_TRUE(threw);
}

TEST(thread_pool_multiple_producers) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};
    const int tasks_per_producer = 50;
    const int num_producers = 4;

    std::vector<std::thread> producers;
    for (int p = 0; p < num_producers; ++p) {
        producers.emplace_back([&pool, &counter, tasks_per_producer]() {
            for (int i = 0; i < tasks_per_producer; ++i) {
                pool.submit([&counter]() {
                    counter.fetch_add(1, std::memory_order_relaxed);
                });
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }

    pool.shutdown();
    ASSERT_EQ(counter.load(), tasks_per_producer * num_producers);
}
