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
#include <istream>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <thread>
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/packet/ConnectionPackets.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/network/ServerLoginNetworkHandler.hpp"
#include "net/minecraft/server/network/ServerSocket.hpp"
namespace {
using net::minecraft::server::MinecraftServer;
using net::minecraft::server::network::ServerLoginNetworkHandler;
using net::minecraft::server::network::ServerSocket;
void ensureWinsock() {
 static std::once_flag once;
 std::call_once(once, []() {
  WSADATA data{};
  if(::WSAStartup(MAKEWORD(2, 2), &data) != 0) {
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
 if(::getaddrinfo("127.0.0.1", portStream.str().c_str(), &hints, &result) != 0 || result == nullptr) {
  return INVALID_SOCKET;
 }
 SOCKET socket = INVALID_SOCKET;
 for(addrinfo* candidate = result; candidate != nullptr; candidate = candidate->ai_next) {
  socket = ::socket(candidate->ai_family, candidate->ai_socktype, candidate->ai_protocol);
  if(socket == INVALID_SOCKET) {
   continue;
  }
  if(::connect(socket, candidate->ai_addr, static_cast<int>(candidate->ai_addrlen)) == 0) {
   break;
  }
  ::closesocket(socket);
  socket = INVALID_SOCKET;
 }
 ::freeaddrinfo(result);
 return socket;
}
[[nodiscard]] SOCKET acceptWithTimeout(ServerSocket& listenSocket, int timeoutMs = 3000) {
 const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
 while(std::chrono::steady_clock::now() < deadline) {
  std::string remote;
  const SOCKET accepted = listenSocket.accept(remote);
  if(accepted != INVALID_SOCKET) {
   return accepted;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
 }
 return INVALID_SOCKET;
}
[[nodiscard]] SOCKET makeLoopbackConnection(ServerSocket& listenSocket) {
 const std::uint16_t port = listenSocket.boundPort();
 std::thread connector([&]() {
  SOCKET client = connectLoopback(port);
  if(client != INVALID_SOCKET) {
   ::shutdown(client, SD_BOTH);
   ::closesocket(client);
  }
 });
 const SOCKET serverSide = acceptWithTimeout(listenSocket);
 connector.join();
 return serverSide;
}
} // namespace
namespace net::minecraft::test {
TEST(ServerLoginNetworkHandler, OutdatedClientRejected) {
 ServerSocket listenSocket;
 listenSocket.bindAndListen("127.0.0.1", 0);
 const SOCKET serverSide = makeLoopbackConnection(listenSocket);
 ASSERT_NE(serverSide, INVALID_SOCKET);
 MinecraftServer server;
 ServerLoginNetworkHandler handler(&server, nullptr, serverSide, "test", false);
 net::minecraft::LoginHelloPacket packet;
 packet.protocolVersion = 13;
 packet.username = "Player";
 handler.onHello(packet);
 EXPECT_TRUE(handler.closed);
}
TEST(ServerLoginNetworkHandler, OfflineHandshakeSendsDash) {
 ServerSocket listenSocket;
 listenSocket.bindAndListen("127.0.0.1", 0);
 const std::uint16_t port = listenSocket.boundPort();
 std::optional<std::string> receivedName;
 std::thread connector([&]() {
  const SOCKET client = connectLoopback(port);
  if(client == INVALID_SOCKET) {
   return;
  }
  net::minecraft::SocketInputStreamBuf inputBuf(client);
  std::istream input(&inputBuf);
  Packet::ensureRegistered();
  if(std::unique_ptr<Packet> packet = Packet::read(input, false); packet != nullptr) {
   if(const auto* handshake = dynamic_cast<net::minecraft::HandshakePacket*>(packet.get())) {
    receivedName = handshake->name;
   }
  }
  ::shutdown(client, SD_BOTH);
  ::closesocket(client);
 });
 const SOCKET serverSide = acceptWithTimeout(listenSocket);
 ASSERT_NE(serverSide, INVALID_SOCKET);
 MinecraftServer server;
 ServerLoginNetworkHandler handler(&server, nullptr, serverSide, "test", false);
 net::minecraft::HandshakePacket handshake;
 handshake.name = "Player";
 handler.onHandshake(handshake);
 for(int tick = 0; tick < 20; ++tick) {
  handler.tick();
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
 }
 connector.join();
 ASSERT_TRUE(receivedName.has_value());
 EXPECT_EQ(receivedName.value(), "-");
}
TEST(ServerLoginNetworkHandler, ProtocolErrorCloses) {
 ServerSocket listenSocket;
 listenSocket.bindAndListen("127.0.0.1", 0);
 const SOCKET serverSide = makeLoopbackConnection(listenSocket);
 ASSERT_NE(serverSide, INVALID_SOCKET);
 MinecraftServer server;
 ServerLoginNetworkHandler handler(&server, nullptr, serverSide, "test", false);
 net::minecraft::KeepAlivePacket keepAlive;
 handler.handle(keepAlive);
 EXPECT_TRUE(handler.closed);
}
TEST(ServerLoginNetworkHandler, LoginTimeout) {
 ServerSocket listenSocket;
 listenSocket.bindAndListen("127.0.0.1", 0);
 const SOCKET serverSide = makeLoopbackConnection(listenSocket);
 ASSERT_NE(serverSide, INVALID_SOCKET);
 MinecraftServer server;
 ServerLoginNetworkHandler handler(&server, nullptr, serverSide, "test", false);
 for(int tick = 0; tick < 601; ++tick) {
  handler.tick();
 }
 EXPECT_TRUE(handler.closed);
}
} // namespace net::minecraft::test
