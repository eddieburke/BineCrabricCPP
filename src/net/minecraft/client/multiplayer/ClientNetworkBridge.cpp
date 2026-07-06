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
#include "net/minecraft/client/multiplayer/ClientNetworkBridge.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/core/WorldSession.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include "net/minecraft/network/Connection.hpp"
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <mutex>
#include <utility>
namespace net::minecraft::client::multiplayer {
namespace {
bool ensureWinsock() {
  static std::once_flag once;
  static bool ready = false;
  std::call_once(once, [] {
    WSADATA data{};
    ready = ::WSAStartup(MAKEWORD(2, 2), &data) == 0;
  });
  return ready;
}
bool connectWithCancellation(SOCKET socket, const sockaddr* address, int addressLength,
                             const std::atomic_bool* canceled) {
  u_long nonBlocking = 1;
  if(::ioctlsocket(socket, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
    return false;
  }
  const int connectResult = ::connect(socket, address, addressLength);
  if(connectResult == SOCKET_ERROR && ::WSAGetLastError() != WSAEWOULDBLOCK) {
    return false;
  }
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
  while(connectResult == SOCKET_ERROR && std::chrono::steady_clock::now() < deadline) {
    if(canceled != nullptr && canceled->load(std::memory_order_acquire)) {
      return false;
    }
    fd_set writable;
    fd_set failed;
    FD_ZERO(&writable);
    FD_ZERO(&failed);
    FD_SET(socket, &writable);
    FD_SET(socket, &failed);
    timeval timeout{0, 100'000};
    const int selected = ::select(0, nullptr, &writable, &failed, &timeout);
    if(selected > 0) {
      int socketError = 0;
      int errorLength = sizeof(socketError);
      if(::getsockopt(socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&socketError), &errorLength) == 0 &&
         socketError == 0 && FD_ISSET(socket, &writable)) {
        break;
      }
      return false;
    }
    if(selected == SOCKET_ERROR) {
      return false;
    }
  }
  if(connectResult == SOCKET_ERROR && std::chrono::steady_clock::now() >= deadline) {
    return false;
  }
  u_long blocking = 0;
  return ::ioctlsocket(socket, FIONBIO, &blocking) != SOCKET_ERROR;
}
SOCKET openClientSocket(const std::string& host, int port, std::string& errorOut,
                        const std::atomic_bool* canceled) {
  if(!ensureWinsock()) {
    errorOut = "WSAStartup failed";
    return INVALID_SOCKET;
  }
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  std::ostringstream portStream;
  portStream << port;
  const std::string portString = portStream.str();
  addrinfo* addressResult = nullptr;
  const int resolveResult = ::getaddrinfo(host.c_str(), portString.c_str(), &hints, &addressResult);
  if(resolveResult != 0 || addressResult == nullptr) {
    errorOut = "Unknown host '" + host + "'";
    return INVALID_SOCKET;
  }
  SOCKET socket = INVALID_SOCKET;
  for(addrinfo* candidate = addressResult; candidate != nullptr; candidate = candidate->ai_next) {
    if(canceled != nullptr && canceled->load(std::memory_order_acquire)) {
      errorOut = "Connection canceled";
      break;
    }
    socket = ::socket(candidate->ai_family, candidate->ai_socktype, candidate->ai_protocol);
    if(socket == INVALID_SOCKET) {
      continue;
    }
    if(!connectWithCancellation(socket, candidate->ai_addr, static_cast<int>(candidate->ai_addrlen), canceled)) {
      ::closesocket(socket);
      socket = INVALID_SOCKET;
      continue;
    }
    break;
  }
  ::freeaddrinfo(addressResult);
  if(socket == INVALID_SOCKET) {
    errorOut = canceled != nullptr && canceled->load(std::memory_order_acquire) ? "Connection canceled"
                                                                                : "Failed to connect";
  }
  return socket;
}
} // namespace
ClientNetworkBridge::ClientNetworkBridge(core::WorldSession* worldSession) noexcept : worldSession_(worldSession) {}
ClientNetworkBridge::~ClientNetworkBridge() = default;
bool ClientNetworkBridge::connect(client::Minecraft* minecraft, const std::string& host, int port,
                                  std::string& errorOut, const std::atomic_bool* canceled) {
  disconnect();
  if(minecraft == nullptr) {
    errorOut = "Internal client error";
    return false;
  }
  const SOCKET socket = openClientSocket(host, port, errorOut, canceled);
  if(socket == INVALID_SOCKET) {
    return false;
  }
  handler_ = std::make_unique<multiplayer::ClientNetworkHandler>(minecraft);
  handler_->message = "Connecting...";
  try {
    connection_ = std::make_unique<net::minecraft::Connection>(socket, "Client", *handler_);
  } catch(const std::exception& error) {
    ::closesocket(socket);
    handler_.reset();
    errorOut = std::string("Internal client error: ") + error.what();
    return false;
  }
  handler_->bindConnection(connection_.get());
  return true;
}
void ClientNetworkBridge::disconnect(const std::string& reason) {
  if(connection_ != nullptr) {
    connection_->disconnect();
  }
  if(handler_ != nullptr) {
    handler_->disconnect(reason);
  }
  if(worldSession_ != nullptr && handler_ != nullptr && handler_->minecraft != nullptr) {
    worldSession_->clearWorld(*handler_->minecraft);
  }
  handler_.reset();
  connection_.reset();
}
void ClientNetworkBridge::tick() {
  if(handler_ != nullptr) {
    handler_->tick();
  }
}
multiplayer::ClientNetworkHandler* ClientNetworkBridge::handler() const noexcept {
  return handler_.get();
}
net::minecraft::Connection* ClientNetworkBridge::connection() const noexcept {
  return connection_.get();
}
} // namespace net::minecraft::client::multiplayer
