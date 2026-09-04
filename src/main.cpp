#include "server/config.hpp"
#include "server/logger.hpp"
#include "server/tcp_server.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
#endif

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <memory>

#ifdef _WIN32
static bool initialize_winsock() {
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != 0) {
        std::fprintf(stderr, "WSAStartup failed: %d\n", result);
        return false;
    }
    return true;
}

static void cleanup_winsock() {
    WSACleanup();
}
#endif

static std::unique_ptr<server::TcpServer> g_server;

#ifdef _WIN32
static BOOL WINAPI console_handler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT) {
        LOG_INFO("Received shutdown signal, stopping server...");
        if (g_server) g_server->stop();
        return TRUE;
    }
    return FALSE;
}
#else
static void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        LOG_INFO("Received shutdown signal, stopping server...");
        if (g_server) g_server->stop();
    }
}
#endif

int main(int argc, char* argv[]) {
    server::Config config = server::Config::parse(argc, argv);

    server::Logger::instance().set_level(server::LogLevel::LOG_INFO);
    LOG_INFO("Starting HTTP server on port " + std::to_string(config.port));

#ifdef _WIN32
    if (!initialize_winsock()) return EXIT_FAILURE;
#endif

#ifdef _WIN32
    SetConsoleCtrlHandler(console_handler, TRUE);
#else
    struct sigaction sa{};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
#endif

    try {
        g_server = std::make_unique<server::TcpServer>(config);
        g_server->start();
    } catch (const std::exception& e) {
        LOG_ERROR("Fatal: " + std::string(e.what()));
    }

    g_server.reset();

#ifdef _WIN32
    cleanup_winsock();
#endif

    LOG_INFO("Server shutdown complete.");
    return EXIT_SUCCESS;
}
