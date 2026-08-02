// WI-9 / HZ-17 regression test: per-Connection read accounting merged on the game thread.
// Before the fix, Packet::read() mutated process-global packetTrackers()/incomingCount()
// directly from every connection's reader thread (a data race on the static unordered_map).
// Now each Connection keeps its own decode-side stats and Connection::tick() merges them on
// the game thread. This test runs several connections' readers concurrently and asserts the
// merged global counters come out exact (no torn/lost/double-counted updates).
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
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/Packet.hpp"
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
struct ConnectionPair {
  net::minecraft::server::network::ServerSocket listener;
  SOCKET clientSocket = INVALID_SOCKET;
  SOCKET serverSocket = INVALID_SOCKET;
  std::unique_ptr<CountingHandler> clientHandler;
  std::unique_ptr<net::minecraft::Connection> client;
  std::unique_ptr<CountingHandler> serverHandler;
  std::unique_ptr<net::minecraft::Connection> server;
};
ConnectionPair makePair() {
 ensureWinsock();
 ConnectionPair pair;
 pair.listener.bindAndListen("127.0.0.1", 0);
 const std::uint16_t port = pair.listener.boundPort();
 std::thread connector([&pair, port]() { pair.clientSocket = connectLoopback(port); });
 std::string remote;
 const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
 while(std::chrono::steady_clock::now() < deadline && pair.serverSocket == INVALID_SOCKET) {
  pair.serverSocket = pair.listener.accept(remote);
  if(pair.serverSocket == INVALID_SOCKET) {
   std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
 }
 connector.join();
 if(pair.clientSocket == INVALID_SOCKET || pair.serverSocket == INVALID_SOCKET) {
  throw std::runtime_error("could not establish loopback connection pair");
 }
 pair.clientHandler = std::make_unique<CountingHandler>(false);
 pair.serverHandler = std::make_unique<CountingHandler>(true);
 pair.client =
     std::make_unique<net::minecraft::Connection>(pair.clientSocket, "AccountingTestClient", *pair.clientHandler);
 pair.server =
     std::make_unique<net::minecraft::Connection>(pair.serverSocket, "AccountingTestServer", *pair.serverHandler);
 return pair;
}
} // namespace
namespace net::minecraft::test {
TEST(PacketAccounting, ConcurrentReadersMergeWithoutTornCounters) {
 Packet::ensureRegistered();
 Packet::resetReadStats();
 constexpr int kConnectionPairs = 4;
 constexpr int kPacketsPerConnection = 200;
 std::vector<ConnectionPair> pairs;
 for(int i = 0; i < kConnectionPairs; ++i) {
  pairs.push_back(makePair());
 }
 for(ConnectionPair& pair : pairs) {
  for(int i = 0; i < kPacketsPerConnection; ++i) {
   pair.server->sendPacket(std::make_unique<KeepAlivePacket>());
  }
 }
 const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
 while(std::chrono::steady_clock::now() < deadline) {
  bool allDrained = true;
  for(ConnectionPair& pair : pairs) {
   pair.client->tick();
   if(pair.clientHandler->applied() < kPacketsPerConnection) {
    allDrained = false;
   }
  }
  if(allDrained) {
   break;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
 }
 for(ConnectionPair& pair : pairs) {
  ASSERT_EQ(pair.clientHandler->applied(), kPacketsPerConnection) << "backlog never fully drained";
 }
 for(ConnectionPair& pair : pairs) {
  pair.client->tick();
 }
 EXPECT_EQ(Packet::incomingReadCount(), kConnectionPairs * kPacketsPerConnection);
 const auto snapshot = Packet::snapshotReadStats();
 const auto it = snapshot.find(0);
 ASSERT_NE(it, snapshot.end());
 EXPECT_EQ(it->second.count(), kConnectionPairs * kPacketsPerConnection);
}
} // namespace net::minecraft::test
