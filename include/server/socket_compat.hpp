#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#endif

#include <cstring>
#include <string>

namespace server {

#ifdef _WIN32
using socket_t = SOCKET;
inline socket_t invalid_socket() { return INVALID_SOCKET; }
#else
using socket_t = int;
inline socket_t invalid_socket() { return -1; }
#endif

inline void close_socket(socket_t sock) {
#ifdef _WIN32
    ::closesocket(sock);
#else
    ::close(sock);
#endif
}

inline int last_socket_error() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

} // namespace server
