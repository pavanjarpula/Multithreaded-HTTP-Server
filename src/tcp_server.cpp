#include "server/tcp_server.hpp"

#include <sstream>
#include <chrono>

namespace server {

TcpServer::TcpServer(const Config& config)
    : config_(config)
    , listen_fd_(invalid_socket())
    , running_(false)
    , pool_(nullptr)
    , router_(config.document_root) {
    router_.setup_default_routes();
}

TcpServer::~TcpServer() {
    stop();
}

void TcpServer::start() {
    listen_fd_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_fd_ == invalid_socket()) {
        throw std::runtime_error("socket() failed: " + std::to_string(last_socket_error()));
    }

    int opt = 1;
    if (::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR,
                     reinterpret_cast<const char*>(&opt), sizeof(opt)) < 0) {
        close_socket(listen_fd_);
        listen_fd_ = invalid_socket();
        throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");
    }

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(config_.port);

    if (::bind(listen_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        close_socket(listen_fd_);
        listen_fd_ = invalid_socket();
        throw std::runtime_error("bind() failed on port " + std::to_string(config_.port));
    }

    if (::listen(listen_fd_, config_.backlog) < 0) {
        close_socket(listen_fd_);
        listen_fd_ = invalid_socket();
        throw std::runtime_error("listen() failed");
    }

    running_.store(true, std::memory_order_release);

    LOG_INFO("Listening on port " + std::to_string(config_.port) +
             " with " + std::to_string(config_.thread_count) + " threads");

    pool_ = std::make_unique<ThreadPool>(config_.thread_count);

    Metrics::instance().set_thread_count(pool_->thread_count());

    accept_loop();

    LOG_INFO("Shutting down thread pool...");
    pool_->shutdown();
    pool_.reset();

    if (listen_fd_ != invalid_socket()) {
        close_socket(listen_fd_);
        listen_fd_ = invalid_socket();
    }

    LOG_INFO("Server stopped.");
}

void TcpServer::stop() {
    if (running_.exchange(false, std::memory_order_acquire)) {
        if (listen_fd_ != invalid_socket()) {
            close_socket(listen_fd_);
            listen_fd_ = invalid_socket();
        }
    }
}

void TcpServer::accept_loop() {
    while (running_.load(std::memory_order_relaxed)) {
        struct sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);

        socket_t client_fd = ::accept(listen_fd_,
            reinterpret_cast<struct sockaddr*>(&client_addr), &addr_len);

        if (client_fd == invalid_socket()) {
            if (!running_.load(std::memory_order_relaxed)) break;
            int err = last_socket_error();
#ifdef _WIN32
            if (err == WSAEINTR) continue;
#else
            if (err == EINTR) continue;
#endif
            LOG_ERROR("accept() failed: error " + std::to_string(err));
            continue;
        }

        Metrics::instance().connection_opened();

        char ip_buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_buf, sizeof(ip_buf));
        std::string client_ip = ip_buf;
        int client_port = ntohs(client_addr.sin_port);
        std::string client_info = client_ip + ":" + std::to_string(client_port);

        socket_t fd_copy = client_fd;
        std::string addr_copy = client_info;

        try {
            pool_->submit([this, fd_copy, addr_copy]() {
                handle_client(fd_copy, addr_copy);
            });
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to submit client task: " + std::string(e.what()));
            close_socket(client_fd);
            Metrics::instance().connection_closed();
        }
    }
}

void TcpServer::handle_client(socket_t client_fd, std::string client_addr) {
    auto start_time = std::chrono::steady_clock::now();

    Metrics::instance().record_request();
    Metrics::instance().set_queue_size(pool_ ? pool_->queue_size() : 0);

#ifdef _WIN32
    DWORD timeout = 10000;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&timeout), sizeof(timeout));
#else
    struct timeval tv{};
    tv.tv_sec = 10;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

    try {
        std::string raw_request = recv_request(client_fd);
        if (raw_request.empty()) {
            close_socket(client_fd);
            Metrics::instance().connection_closed();
            return;
        }

        HttpParser::ParseResult parse_result = HttpParser::parse(
            raw_request, config_.max_header_size, config_.max_body_size);

        HttpResponse response;

        switch (parse_result.error) {
            case HttpParser::ParseResult::Error::INCOMPLETE:
                response = HttpResponse::bad_request("Incomplete request");
                Metrics::instance().record_client_error();
                break;
            case HttpParser::ParseResult::Error::MALFORMED:
                response = HttpResponse::bad_request("Malformed request");
                Metrics::instance().record_client_error();
                break;
            case HttpParser::ParseResult::Error::HEADER_TOO_LARGE:
                response = HttpResponse::payload_too_large();
                Metrics::instance().record_client_error();
                break;
            case HttpParser::ParseResult::Error::BODY_TOO_LARGE:
                response = HttpResponse::payload_too_large();
                Metrics::instance().record_client_error();
                break;
            case HttpParser::ParseResult::Error::NONE:
            {
                HttpRequest http_req = HttpParser::build_request(parse_result);
                response = router_.handle(http_req);
                int code = static_cast<int>(response.status_code());
                if (code >= 400 && code < 500) {
                    Metrics::instance().record_client_error();
                } else if (code >= 500) {
                    Metrics::instance().record_server_error();
                } else {
                    Metrics::instance().record_success();
                }
                break;
            }
        }

        std::string serialized = response.serialize();
        send_all(client_fd, serialized.data(), serialized.size());
        Metrics::instance().add_bytes_sent(serialized.size());

        auto end_time = std::chrono::steady_clock::now();
        double latency_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        Metrics::instance().record_latency(latency_ms);

        std::ostringstream log_msg;
        log_msg << parse_result.method
                << " " << parse_result.path
                << " -> " << static_cast<int>(response.status_code())
                << " (" << std::fixed << std::setprecision(1) << latency_ms << " ms)"
                << " [" << client_addr << "]";
        LOG_INFO(log_msg.str());

    } catch (const std::exception& e) {
        LOG_ERROR("Error handling client " + client_addr + ": " + e.what());
        Metrics::instance().record_server_error();

        HttpResponse error_resp = HttpResponse::internal_error();
        std::string serialized = error_resp.serialize();
        send_all(client_fd, serialized.data(), serialized.size());
    }

    close_socket(client_fd);
    Metrics::instance().connection_closed();
}

bool TcpServer::send_all(socket_t fd, const char* data, std::size_t len) {
    std::size_t total_sent = 0;
    while (total_sent < len) {
        auto sent = ::send(fd, data + total_sent,
                          static_cast<int>(len - total_sent), 0);
        if (sent <= 0) {
            int err = last_socket_error();
#ifdef _WIN32
            if (err == WSAEINTR) continue;
#else
            if (err == EINTR) continue;
#endif
            return false;
        }
        total_sent += static_cast<std::size_t>(sent);
    }
    return true;
}

std::string TcpServer::recv_request(socket_t fd) {
    std::string request;
    request.reserve(4096);
    char buffer[4096];

    while (true) {
        auto n = ::recv(fd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            if (n == 0) return request;
            int err = last_socket_error();
#ifdef _WIN32
            if (err == WSAEINTR || err == WSAEWOULDBLOCK) continue;
#else
            if (err == EINTR || err == EAGAIN) continue;
#endif
            return request;
        }

        buffer[n] = '\0';
        request.append(buffer, static_cast<std::size_t>(n));

        if (request.find("\r\n\r\n") != std::string::npos) {
            std::string lower = request;
            for (std::size_t i = 0; i < lower.size(); ++i) {
                lower[i] = static_cast<char>(std::tolower(lower[i]));
            }

            std::size_t cl_pos = lower.find("content-length:");
            if (cl_pos == std::string::npos) {
                return request;
            }

            std::size_t colon_pos = request.find(':', cl_pos);
            std::size_t eol_pos = request.find("\r\n", colon_pos);
            if (colon_pos == std::string::npos || eol_pos == std::string::npos) {
                return request;
            }

            std::string cl_value = request.substr(colon_pos + 1, eol_pos - colon_pos - 1);
            std::size_t ws = cl_value.find_first_not_of(" \t");
            if (ws != std::string::npos) cl_value = cl_value.substr(ws);

            try {
                std::size_t content_length = static_cast<std::size_t>(std::stoull(cl_value));
                std::size_t header_size = request.find("\r\n\r\n") + 4;
                std::size_t body_so_far = request.size() - header_size;

                while (body_so_far < content_length && body_so_far < config_.max_body_size) {
                    n = ::recv(fd, buffer, sizeof(buffer) - 1, 0);
                    if (n <= 0) break;
                    buffer[n] = '\0';
                    request.append(buffer, static_cast<std::size_t>(n));
                    body_so_far = request.size() - header_size;
                }
            } catch (...) {
            }

            return request;
        }

        if (request.size() > config_.max_header_size) {
            return request;
        }
    }
}

} // namespace server
