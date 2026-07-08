#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include <gtest/gtest.h>

#include <chrono>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>

#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/network/ConnectionListener.hpp"

namespace {
using net::minecraft::server::MinecraftServer;
using net::minecraft::server::network::ConnectionListener;

void ensureWinsock() {
    static std::once_flag once;
    std::call_once(once, []() {
        WSADATA data{};
        if (::WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
    });
}

SOCKET connectLoopback(std::uint16_t port) {
    ensureWinsock();
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    addrinfo* result = nullptr;
    std::ostringstream portStream;
    portStream << port;
    if (::getaddrinfo("127.0.0.1", portStream.str().c_str(), &hints, &result) != 0 || result == nullptr) {
        return INVALID_SOCKET;
    }
    SOCKET socket = INVALID_SOCKET;
    for (addrinfo* candidate = result; candidate != nullptr; candidate = candidate->ai_next) {
        socket = ::socket(candidate->ai_family, candidate->ai_socktype, candidate->ai_protocol);
        if (socket == INVALID_SOCKET) {
            continue;
        }
        if (::connect(socket, candidate->ai_addr, static_cast<int>(candidate->ai_addrlen)) == 0) {
            break;
        }
        ::closesocket(socket);
        socket = INVALID_SOCKET;
    }
    ::freeaddrinfo(result);
    return socket;
}
}  // namespace

namespace net::minecraft::test {
TEST(ConnectionListener, BindAndAcceptSmoke) {
    MinecraftServer server;
    ConnectionListener listener(&server, "127.0.0.1", 0, false);
    const std::uint16_t port = listener.boundPort();
    ASSERT_NE(port, 0);
    const SOCKET client = connectLoopback(port);
    ASSERT_NE(client, INVALID_SOCKET);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    listener.tick();
    listener.close();
    ::shutdown(client, SD_BOTH);
    ::closesocket(client);
}
}  // namespace net::minecraft::test
