#pragma once
#ifdef _WIN32
#if !defined(_WINSOCK2API_) && !defined(_WINSOCKAPI_)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#endif
#include <cstdint>
#include <string>
namespace net::minecraft::server::network {
class ServerSocket {
 public:
 ServerSocket() = default;
 ServerSocket(const ServerSocket&) = delete;
 ServerSocket& operator=(const ServerSocket&) = delete;
 ServerSocket(ServerSocket&& other) noexcept;
 ServerSocket& operator=(ServerSocket&& other) noexcept;
 ~ServerSocket();
 static void ensureWinsock();
 void bindAndListen(const std::string& bindAddress, std::uint16_t port, int backlog = SOMAXCONN);
 [[nodiscard]] SOCKET accept(std::string& remoteAddressOut);
 [[nodiscard]] bool isOpen() const noexcept {
  return socket_ != INVALID_SOCKET;
 }
 [[nodiscard]] SOCKET handle() const noexcept {
  return socket_;
 }
 [[nodiscard]] std::uint16_t boundPort() const;
 void close();
 static void configureAcceptedSocket(SOCKET socket);

 private:
 SOCKET socket_ = INVALID_SOCKET;
};
} // namespace net::minecraft::server::network
