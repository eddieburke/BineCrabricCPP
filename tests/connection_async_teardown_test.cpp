// WI-10 / H1 regression test: Connection::disconnect() must be async -- it must not join the
// reader/writer threads on the caller's stack. Against a black-holed peer (accepts but never
// reads, never sends) the old code's disconnect() blocked in joinThreads() for up to the 30s
// SO_SNDTIMEO; the new code CASes open_ false, unblocks recv, and returns immediately. The
// reader/writer threads then observe close and are reclaimed (watchdog SD_BOTH + join) when
// the Connection is destroyed.
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
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/packet/ChunkPackets.hpp"
#include "net/minecraft/network/packet/ConnectionPackets.hpp"
#include "net/minecraft/server/network/ServerSocket.hpp"
namespace {
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
class CountingHandler : public net::minecraft::NetworkHandler {
 public:
  explicit CountingHandler(bool serverSide) : serverSide_(serverSide) {
  }
  [[nodiscard]] bool isServerSide() const override {
   return serverSide_;
  }
  void onKeepAlive(const net::minecraft::KeepAlivePacket&) override {
   applied_.fetch_add(1, std::memory_order_relaxed);
  }

 private:
  bool serverSide_;
  std::atomic<int> applied_{0};
};
std::vector<std::uint8_t> makeFakeChunkData(int seed) {
 constexpr std::size_t kChunkVolume = 16 * 128 * 16;
 constexpr std::size_t kDecompressedSize = kChunkVolume * 5 / 2;
 std::vector<std::uint8_t> data(kDecompressedSize);
 std::mt19937 rng(seed);
 for(auto& byte : data) {
  byte = static_cast<std::uint8_t>(rng() & 0xFF);
 }
 return data;
}
} // namespace
namespace net::minecraft::test {
TEST(ConnectionAsyncTeardown, DisconnectReturnsWithoutJoiningAgainstBlackHole) {
 server::network::ServerSocket listener;
 listener.bindAndListen("127.0.0.1", 0);
 const std::uint16_t port = listener.boundPort();
 SOCKET clientSocket = INVALID_SOCKET;
 std::thread connector([&]() { clientSocket = connectLoopback(port); });
 std::string remote;
 SOCKET peerSocket = INVALID_SOCKET;
 const auto acceptDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
 while(std::chrono::steady_clock::now() < acceptDeadline && peerSocket == INVALID_SOCKET) {
  peerSocket = listener.accept(remote);
  if(peerSocket == INVALID_SOCKET) {
   std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
 }
 connector.join();
 ASSERT_NE(clientSocket, INVALID_SOCKET);
 ASSERT_NE(peerSocket, INVALID_SOCKET);
 long long disconnectMs = 0;
 long long destroyMs = 0;
 std::chrono::steady_clock::time_point destroyStart;
 {
  CountingHandler handler(false);
  Connection connection(clientSocket, "AsyncTeardownClient", handler);
  for(int i = 0; i < 200; ++i) {
   auto packet = std::make_unique<ChunkDataS2CPacket>();
   packet->x = i * 16;
   packet->y = 0;
   packet->z = 0;
   packet->sizeX = 16;
   packet->sizeY = 128;
   packet->sizeZ = 16;
   packet->chunkData = makeFakeChunkData(i);
   connection.sendPacket(std::move(packet));
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  const auto disconnectStart = std::chrono::steady_clock::now();
  connection.disconnect();
  disconnectMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::steady_clock::now() - disconnectStart)
                     .count();
  destroyStart = std::chrono::steady_clock::now();
 }
 destroyMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - destroyStart)
                 .count();
 ::shutdown(peerSocket, SD_BOTH);
 ::closesocket(peerSocket);
 EXPECT_LT(disconnectMs, 250) << "disconnect() joined reader/writer on the caller's stack";
 EXPECT_LT(destroyMs, 3000) << "Connection destruction stalled on a dead peer";
 const auto threadDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
 while(std::chrono::steady_clock::now() < threadDeadline &&
       (Connection::getReadThreadCount() > 0 || Connection::getWriteThreadCount() > 0)) {
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
 }
 EXPECT_EQ(Connection::getReadThreadCount(), 0) << "reader thread never reclaimed";
 EXPECT_EQ(Connection::getWriteThreadCount(), 0) << "writer thread never reclaimed";
}
} // namespace net::minecraft::test
