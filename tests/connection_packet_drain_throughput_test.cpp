// Measures how many packets Connection::tick() actually drains per call under a large
// backlog, over a real loopback socket. This exists to turn "server lag that isn't the
// server's fault" from a guess into a number: Connection::tick's adaptive drain
// (kMinDrain=8, kMaxDrain=4096, 3ms wall-clock budget -- see Connection.cpp) is supposed to
// scale up past the 8-packet floor when a backlog is waiting, so a slow (e.g. unoptimized
// -O0) build should still drain hundreds of packets per tick, not get stuck near the floor.
// If a future change regresses that adaptive behavior back to a fixed small drain, this
// test's throughput assertion should catch it before it reaches a live join.
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
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <thread>
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/NetworkHandler.hpp"
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
 [[nodiscard]] int applied() const {
  return applied_.load(std::memory_order_relaxed);
 }

 private:
 bool serverSide_;
 std::atomic<int> applied_{0};
};
} // namespace
namespace net::minecraft::test {
TEST(ConnectionPacketDrainThroughput, TickDrainsFarMoreThanTheMinimumUnderBacklog) {
 server::network::ServerSocket listenSocket;
 listenSocket.bindAndListen("127.0.0.1", 0);
 ASSERT_TRUE(listenSocket.isOpen());
 const std::uint16_t port = listenSocket.boundPort();
 ASSERT_NE(port, 0);
 SOCKET clientSocket = INVALID_SOCKET;
 std::thread connector([&]() { clientSocket = connectLoopback(port); });
 std::string remoteAddress;
 SOCKET serverSocket = INVALID_SOCKET;
 const auto acceptDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
 while(std::chrono::steady_clock::now() < acceptDeadline && serverSocket == INVALID_SOCKET) {
  serverSocket = listenSocket.accept(remoteAddress);
  if(serverSocket == INVALID_SOCKET) {
   std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
 }
 connector.join();
 ASSERT_NE(serverSocket, INVALID_SOCKET);
 ASSERT_NE(clientSocket, INVALID_SOCKET);
 CountingHandler serverHandler(true);
 CountingHandler clientHandler(false);
 Connection serverConnection(serverSocket, "DrainTestServer", serverHandler);
 Connection clientConnection(clientSocket, "DrainTestClient", clientHandler);
 constexpr int kPacketCount = 2000;
 for(int i = 0; i < kPacketCount; ++i) {
  serverConnection.sendPacket(std::make_unique<KeepAlivePacket>());
 }
 // KeepAlivePacket is not a worldPacket, so it goes on the immediate sendQueue_ (no public
 // accessor to poll it directly); the writer thread streams it out continuously. Give it a
 // head start so the client's readQueue_ already has a backlog once we start measuring
 // tick()'s drain rate below, rather than measuring an empty-queue steady state.
 std::this_thread::sleep_for(std::chrono::milliseconds(300));
 int ticks = 0;
 int maxAppliedInOneTick = 0;
 const auto drainStart = std::chrono::steady_clock::now();
 const auto drainDeadline = drainStart + std::chrono::seconds(15);
 while(std::chrono::steady_clock::now() < drainDeadline && clientHandler.applied() < kPacketCount) {
  const int before = clientHandler.applied();
  clientConnection.tick();
  ++ticks;
  maxAppliedInOneTick = std::max(maxAppliedInOneTick, clientHandler.applied() - before);
 }
 const auto drainWallMs =
     std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - drainStart).count();
 std::printf(
     "[ConnectionPacketDrainThroughput] applied=%d/%d ticks=%d maxAppliedInOneTick=%d wallMs=%lld avgPerTick=%.1f\n",
     clientHandler.applied(),
     kPacketCount,
     ticks,
     maxAppliedInOneTick,
     static_cast<long long>(drainWallMs),
     ticks > 0 ? static_cast<double>(clientHandler.applied()) / static_cast<double>(ticks) : 0.0);
 std::fflush(stdout);
 ASSERT_EQ(clientHandler.applied(), kPacketCount) << "backlog never fully drained within the deadline";
 // Java's Connection.tick() drains up to 101 packets unconditionally every tick; this port's
 // budget is adaptive (time-boxed, not count-boxed) but should still clear a large backlog
 // in single digits of ticks, not hundreds -- if it takes many hundreds of ticks to drain a
 // 2000-packet burst, the 3ms/tick wall-clock budget (not the join handshake) is the reason
 // a joining player sees "server lag" during a chunk burst.
 EXPECT_LT(ticks, 200) << "drain took far more ticks than expected -- packet backlog is likely visible as lag";
 EXPECT_GT(maxAppliedInOneTick, 8) << "a single tick never drained past kMinDrain=8 even under backlog";
}
} // namespace net::minecraft::test
