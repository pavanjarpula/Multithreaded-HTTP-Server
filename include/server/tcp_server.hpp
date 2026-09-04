#pragma once

#include "socket_compat.hpp"
#include "config.hpp"
#include "thread_pool.hpp"
#include "router.hpp"
#include "http_parser.hpp"
#include "http_response.hpp"
#include "metrics.hpp"
#include "logger.hpp"

#include <atomic>
#include <string>
#include <memory>

namespace server {

class TcpServer {
public:
    explicit TcpServer(const Config& config);
    ~TcpServer();

    void start();
    void stop();

    std::size_t queue_size() const { return pool_ ? pool_->queue_size() : 0; }
    std::size_t queue_capacity() const { return pool_ ? pool_->queue_capacity() : 0; }
    std::size_t thread_count() const { return pool_ ? pool_->thread_count() : 0; }
    std::size_t active_workers() const { return pool_ ? pool_->active_workers() : 0; }
    bool is_queue_full() const { return pool_ ? pool_->is_queue_full() : false; }

    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

private:
    void accept_loop();
    void handle_client(socket_t client_fd, std::string client_addr);
    bool send_all(socket_t fd, const char* data, std::size_t len);
    std::string recv_request(socket_t fd);

    Config config_;
    socket_t listen_fd_;
    std::atomic<bool> running_;
    std::unique_ptr<ThreadPool> pool_;
    Router router_;
};

} // namespace server
