#include "net/minecraft/server/network/ServerSocket.hpp"
#include <mutex>
#include <sstream>
#include <stdexcept>
namespace net::minecraft::server::network {
namespace {
std::string formatPeerAddress(SOCKET socket) {
 sockaddr_storage storage{};
 int length = sizeof(storage);
 if(::getpeername(socket, reinterpret_cast<sockaddr*>(&storage), &length) != 0) {
  return "unknown";
 }
 char host[NI_MAXHOST]{};
 char service[NI_MAXSERV]{};
 if(::getnameinfo(reinterpret_cast<sockaddr*>(&storage),
                  length,
                  host,
                  sizeof(host),
                  service,
                  sizeof(service),
                  NI_NUMERICHOST | NI_NUMERICSERV) != 0) {
  return "unknown";
 }
 return std::string(host) + ":" + service;
}
} // namespace
ServerSocket::ServerSocket(ServerSocket&& other) noexcept : socket_(other.socket_) {
 other.socket_ = INVALID_SOCKET;
}
ServerSocket& ServerSocket::operator=(ServerSocket&& other) noexcept {
 if(this != &other) {
  close();
  socket_ = other.socket_;
  other.socket_ = INVALID_SOCKET;
 }
 return *this;
}
ServerSocket::~ServerSocket() {
 close();
}
void ServerSocket::ensureWinsock() {
 static std::once_flag once;
 std::call_once(once, []() {
  WSADATA data{};
  if(::WSAStartup(MAKEWORD(2, 2), &data) != 0) {
   throw std::runtime_error("WSAStartup failed");
  }
 });
}
void ServerSocket::bindAndListen(const std::string& bindAddress, std::uint16_t port, int backlog) {
 close();
 ensureWinsock();
 addrinfo hints{};
 hints.ai_family = AF_UNSPEC;
 hints.ai_socktype = SOCK_STREAM;
 hints.ai_protocol = IPPROTO_TCP;
 hints.ai_flags = AI_PASSIVE;
 std::ostringstream portStream;
 portStream << port;
 const std::string portString = portStream.str();
 addrinfo* addressResult = nullptr;
 const int resolveResult =
     ::getaddrinfo(bindAddress.empty() ? nullptr : bindAddress.c_str(), portString.c_str(), &hints, &addressResult);
 if(resolveResult != 0 || addressResult == nullptr) {
  throw std::runtime_error("Failed to resolve bind address '" + bindAddress + "'");
 }
 SOCKET listenSocket = INVALID_SOCKET;
 for(addrinfo* candidate = addressResult; candidate != nullptr; candidate = candidate->ai_next) {
  listenSocket = ::socket(candidate->ai_family, candidate->ai_socktype, candidate->ai_protocol);
  if(listenSocket == INVALID_SOCKET) {
   continue;
  }
  const BOOL reuse = TRUE;
  ::setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
  // An empty bind address resolves to IPv6 [::] first, which defaults to IPV6_V6ONLY on
  // Windows. IPv4 clients cannot reach an IPv6-only listener.
  if(candidate->ai_family == AF_INET6) {
   const DWORD v6Only = 0;
   ::setsockopt(
       listenSocket, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char*>(&v6Only), sizeof(v6Only));
  }
  if(::bind(listenSocket, candidate->ai_addr, static_cast<int>(candidate->ai_addrlen)) == SOCKET_ERROR) {
   ::closesocket(listenSocket);
   listenSocket = INVALID_SOCKET;
   continue;
  }
  break;
 }
 ::freeaddrinfo(addressResult);
 if(listenSocket == INVALID_SOCKET) {
  throw std::runtime_error("Failed to bind server socket");
 }
 if(::listen(listenSocket, backlog) == SOCKET_ERROR) {
  ::closesocket(listenSocket);
  throw std::runtime_error("Failed to listen on server socket");
 }
 socket_ = listenSocket;
}
SOCKET ServerSocket::accept(std::string& remoteAddressOut) {
 if(socket_ == INVALID_SOCKET) {
  return INVALID_SOCKET;
 }
 const SOCKET client = ::accept(socket_, nullptr, nullptr);
 if(client == INVALID_SOCKET) {
  return INVALID_SOCKET;
 }
 configureAcceptedSocket(client);
 remoteAddressOut = formatPeerAddress(client);
 return client;
}
void ServerSocket::close() {
 if(socket_ != INVALID_SOCKET) {
  ::shutdown(socket_, SD_BOTH);
  ::closesocket(socket_);
  socket_ = INVALID_SOCKET;
 }
}
std::uint16_t ServerSocket::boundPort() const {
 if(socket_ == INVALID_SOCKET) {
  return 0;
 }
 sockaddr_storage storage{};
 int length = sizeof(storage);
 if(::getsockname(socket_, reinterpret_cast<sockaddr*>(&storage), &length) != 0) {
  return 0;
 }
 if(storage.ss_family == AF_INET) {
  return ntohs(reinterpret_cast<const sockaddr_in*>(&storage)->sin_port);
 }
 if(storage.ss_family == AF_INET6) {
  return ntohs(reinterpret_cast<const sockaddr_in6*>(&storage)->sin6_port);
 }
 return 0;
}
void ServerSocket::configureAcceptedSocket(SOCKET socket) {
 const BOOL trueValue = TRUE;
 ::setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&trueValue), sizeof(trueValue));
 const int recvTimeoutMs = 30'000;
 const int sendTimeoutMs = 30'000;
 ::setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&recvTimeoutMs), sizeof(recvTimeoutMs));
 ::setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&sendTimeoutMs), sizeof(sendTimeoutMs));
 const int trafficClass = 24;
 ::setsockopt(socket, IPPROTO_IP, IP_TOS, reinterpret_cast<const char*>(&trafficClass), sizeof(trafficClass));
}
} // namespace net::minecraft::server::network
