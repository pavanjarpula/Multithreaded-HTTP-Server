#pragma once

#include <string>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <thread>

namespace server {

struct Config {
    uint16_t port = 8080;
    unsigned int thread_count = std::thread::hardware_concurrency();
    std::size_t queue_capacity = 50;
    std::string document_root = "public";
    std::size_t max_header_size = 64 * 1024;       // 64 KB
    std::size_t max_body_size = 1 * 1024 * 1024;   // 1 MB
    int backlog = 128;

    static Config parse(int argc, char* argv[]) {
        Config cfg;

        const char* env_port = std::getenv("PORT");
        if (env_port) {
            try {
                cfg.port = static_cast<uint16_t>(std::stoi(env_port));
            } catch (...) {}
        }

        const char* env_cap = std::getenv("QUEUE_CAPACITY");
        if (env_cap) {
            try {
                cfg.queue_capacity = static_cast<std::size_t>(std::stoul(env_cap));
            } catch (...) {}
        }

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--port" && i + 1 < argc) {
                cfg.port = static_cast<uint16_t>(std::stoi(argv[++i]));
            } else if (arg == "--threads" && i + 1 < argc) {
                cfg.thread_count = static_cast<unsigned int>(std::stoi(argv[++i]));
            } else if (arg == "--capacity" && i + 1 < argc) {
                cfg.queue_capacity = static_cast<std::size_t>(std::stoul(argv[++i]));
            } else if (arg == "--root" && i + 1 < argc) {
                cfg.document_root = argv[++i];
            } else if (arg == "--help" || arg == "-h") {
                print_usage(argv[0]);
                std::exit(0);
            }
        }
        if (cfg.thread_count == 0) cfg.thread_count = 2;
        return cfg;
    }

private:
    static void print_usage(const char* program) {
        std::fprintf(stderr,
            "Usage: %s [options]\n"
            "  --port PORT      Listen port (default: 8080)\n"
            "  --threads N      Worker thread count (default: hardware concurrency)\n"
            "  --capacity N     Queue capacity for backpressure (default: 50)\n"
            "  --root PATH      Document root for static files (default: public)\n"
            "  --help, -h       Show this message\n",
            program);
    }
};

} // namespace server
