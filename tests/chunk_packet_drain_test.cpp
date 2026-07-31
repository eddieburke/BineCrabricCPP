// Measures how many ChunkDataS2CPackets Connection::tick() actually drains per call under
// a large backlog, over a real loopback socket, using realistic 16x128x16 chunk payloads
// (~80 KB decompressed, zlib-compressed on the wire). This is the heavy-packet complement
// to connection_packet_drain_throughput_test.cpp (which uses trivial KeepAlivePackets).
//
// The existing KeepAlive test proved the queue/lock/loop machinery itself is not the
// bottleneck — it drained 2000 packets in a single tick(). But KeepAlivePacket::apply() is
// a no-op; real chunk packets go through Packet::read() -> zlibDecompress() on the read
// thread, and handleChunkData() on the tick thread. This test isolates the tick-thread
// cost: the read thread does the decompression, and we measure how fast tick() can apply
// the already-decompressed packets.
//
// H2 hypothesis: in a Debug (-O0) build, if each ChunkDataS2CPacket::apply() or the
// overhead of moving large packets through the drain loop costs significant wall time, the
// 3ms/kMinDrain=8 budget in Connection.cpp:195-220 could limit throughput to a few chunks
// per tick — reproducing exactly the "extreme lag during join" symptom. In a Release (-O3)
// build, apply() should be near-free (our handler just increments a counter), so the drain
// should clear the entire backlog in very few ticks. Running this test in both builds
// quantifies the gap directly.
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
#include <random>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/packet/ChunkPackets.hpp"
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
class ChunkCountingHandler : public net::minecraft::NetworkHandler {
 public:
 explicit ChunkCountingHandler(bool serverSide) : serverSide_(serverSide) {}
 [[nodiscard]] bool isServerSide() const override {
  return serverSide_;
 }
 void handleChunkData(const net::minecraft::ChunkDataS2CPacket& packet) override {
  applied_.fetch_add(1, std::memory_order_relaxed);
  totalBytes_.fetch_add(packet.chunkData.size(), std::memory_order_relaxed);
 }
 [[nodiscard]] int applied() const {
  return applied_.load(std::memory_order_relaxed);
 }
 [[nodiscard]] std::size_t totalBytes() const {
  return totalBytes_.load(std::memory_order_relaxed);
 }

 private:
 bool serverSide_;
 std::atomic<int> applied_{0};
 std::atomic<std::size_t> totalBytes_{0};
};
std::vector<std::uint8_t> makeFakeChunkData(int seed) {
 // 16x128x16 chunk: blocks (1 byte each) + metadata (nibble) + blockLight (nibble) + skyLight (nibble)
 // = 32768 + 16384 + 16384 + 16384 = 81920 bytes
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
TEST(ChunkPacketDrainThroughput, TickDrainsChunkPacketsUnderBacklog) {
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
 ChunkCountingHandler serverHandler(true);
 ChunkCountingHandler clientHandler(false);
 Connection serverConnection(serverSocket, "ChunkDrainTestServer", serverHandler);
 Connection clientConnection(clientSocket, "ChunkDrainTestClient", clientHandler);
 constexpr int kPacketCount = 50;
 for(int i = 0; i < kPacketCount; ++i) {
  auto packet = std::make_unique<ChunkDataS2CPacket>();
  packet->x = i * 16;
  packet->y = 0;
  packet->z = 0;
  packet->sizeX = 16;
  packet->sizeY = 128;
  packet->sizeZ = 16;
  packet->chunkData = makeFakeChunkData(i);
  // compressForSend() is called automatically by Connection::sendPacket()
  serverConnection.sendPacket(std::move(packet));
 }
 // Let the reader thread parse all packets (including zlib decompress on the read side).
 // Chunk packets are ~80 KB decompressed; give enough time for 50 of them to be parsed.
 std::this_thread::sleep_for(std::chrono::seconds(5));
 int ticks = 0;
 int maxAppliedInOneTick = 0;
 const auto drainStart = std::chrono::steady_clock::now();
 const auto drainDeadline = drainStart + std::chrono::seconds(30);
 while(std::chrono::steady_clock::now() < drainDeadline && clientHandler.applied() < kPacketCount) {
  const int before = clientHandler.applied();
  clientConnection.tick();
  ++ticks;
  maxAppliedInOneTick = std::max(maxAppliedInOneTick, clientHandler.applied() - before);
 }
 const auto drainWallMs =
     std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - drainStart).count();
 std::printf(
     "[ChunkPacketDrainThroughput] applied=%d/%d ticks=%d maxAppliedInOneTick=%d wallMs=%lld "
     "totalBytes=%lld avgPerTick=%.1f\n",
     clientHandler.applied(),
     kPacketCount,
     ticks,
     maxAppliedInOneTick,
     static_cast<long long>(drainWallMs),
     static_cast<long long>(clientHandler.totalBytes()),
     ticks > 0 ? static_cast<double>(clientHandler.applied()) / static_cast<double>(ticks) : 0.0);
 std::fflush(stdout);
 ASSERT_EQ(clientHandler.applied(), kPacketCount) << "backlog never fully drained within the deadline";
 // Unlike the KeepAlive test, we don't assert tick counts or maxAppliedInOneTick here —
 // the whole point is to MEASURE the difference between Debug and Release. A Debug build
 // might only drain 1-2 chunk packets per tick (each apply + loop overhead eating the 3ms
 // budget), while Release drains them all in a handful of ticks. Print the numbers so a
 // side-by-side comparison is possible.
}
} // namespace net::minecraft::test
