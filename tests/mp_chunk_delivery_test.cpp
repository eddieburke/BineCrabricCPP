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
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/packet/Packets.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/host/ServerLaunchConfig.hpp"
namespace {
using net::minecraft::Connection;
using net::minecraft::NetworkHandler;
std::atomic<unsigned long long> gTempPathCounter{0};
std::filesystem::path makeTempWorldRoot(const std::string& name) {
  const auto suffix = std::to_string(++gTempPathCounter);
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "minecraft_native_server_tests" / (name + "_" + suffix);
  std::error_code error;
  std::filesystem::remove_all(root, error);
  std::filesystem::create_directories(root);
  return root;
}
std::filesystem::path makeTempAppDataRoot(const std::string& name) {
  const auto suffix = std::to_string(++gTempPathCounter);
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "minecraft_native_server_tests" / (name + "_" + suffix);
  std::error_code error;
  std::filesystem::remove_all(root, error);
  std::filesystem::create_directories(root);
  return root;
}
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
class ProbeClientHandler : public NetworkHandler {
public:
  std::string username = "ProbeClient";
  Connection* connection = nullptr;
  std::atomic<bool> loggedIn{false};
  std::atomic<bool> disconnected{false};
  std::atomic<int> teleports{0};
  std::atomic<int> chunkStatusLoads{0};
  std::atomic<int> chunkDataPackets{0};
  std::atomic<int> keepAlives{0};
  std::string disconnectReason;
  double lastX = 0.0;
  double lastFeetY = 0.0;
  double lastStanceY = 0.0;
  double lastZ = 0.0;
  [[nodiscard]] bool isServerSide() const override {
    return false;
  }
  void onHandshake(const net::minecraft::HandshakePacket& packet) override {
    std::string name = packet.name;
    const std::size_t modsMarker = name.find(";mods=");
    if(modsMarker != std::string::npos) {
      name = name.substr(0, modsMarker);
      auto sync = std::make_unique<net::minecraft::LuaModSyncPacket>();
      sync->kind = net::minecraft::LuaModSyncKind::ClientModList;
      connection->sendPacket(std::move(sync));
    }
    connection->sendPacket(
        std::make_unique<net::minecraft::LoginHelloPacket>(username, net::minecraft::kProtocolVersionBeta173));
  }
  void onHello(const net::minecraft::LoginHelloPacket&) override {
    loggedIn = true;
  }
  void onPlayerMove(const net::minecraft::PlayerMovePacket& packet) override {
    lastX = packet.x;
    lastStanceY = packet.stance;
    lastFeetY = packet.feetY;
    lastZ = packet.z;
    ++teleports;
    auto echo = std::make_unique<net::minecraft::PlayerMoveFullPacket>();
    echo->setMove(packet.x, packet.feetY, packet.stance, packet.z, packet.yaw, packet.pitch, packet.onGround);
    connection->sendPacket(std::move(echo));
  }
  void onChunkStatusUpdate(const net::minecraft::ChunkStatusUpdateS2CPacket& packet) override {
    if(packet.load) {
      ++chunkStatusLoads;
    }
  }
  void handleChunkData(const net::minecraft::ChunkDataS2CPacket&) override {
    ++chunkDataPackets;
  }
  void onKeepAlive(const net::minecraft::KeepAlivePacket&) override {
    ++keepAlives;
    connection->sendPacket(std::make_unique<net::minecraft::KeepAlivePacket>());
  }
  void onDisconnect(const net::minecraft::DisconnectPacket& packet) override {
    disconnectReason = packet.reason;
    disconnected = true;
  }
  void onDisconnected(const std::string& reason, const std::vector<std::string>&) override {
    if(disconnectReason.empty()) {
      disconnectReason = reason;
    }
    disconnected = true;
  }
};
} // namespace
namespace net::minecraft::test {
TEST(MultiplayerChunkDelivery, ServerDeliversChunksToSocketClient) {
  const std::filesystem::path appDataRoot = makeTempAppDataRoot("mp_chunk_delivery_appdata");
#ifdef _WIN32
  _putenv_s("APPDATA", appDataRoot.string().c_str());
  _putenv_s("USERPROFILE", appDataRoot.string().c_str());
#endif
  net::minecraft::mod::runtime::host().shutdown();
  net::minecraft::server::host::ServerLaunchConfig config;
  config.storageRoot = makeTempWorldRoot("mp_chunk_delivery");
  config.worldName = "ChunkDeliveryWorld";
  config.worldSeed = 123456789ULL;
  config.bindAddress = "127.0.0.1";
  config.port = 0;
  config.onlineMode = false;
  config.useConsoleThread = false;
  config.useGui = false;
  net::minecraft::server::MinecraftServer server(config);
  ASSERT_TRUE(server.startAsync()) << (server.lastError().empty() ? "server failed to start" : server.lastError());
  const std::uint16_t port = server.boundPort();
  ASSERT_NE(port, 0);
  const SOCKET socket = connectLoopback(port);
  ASSERT_NE(socket, INVALID_SOCKET);
  ProbeClientHandler handler;
  Connection connection(socket, "Probe", handler);
  handler.connection = &connection;
  connection.sendPacket(std::make_unique<net::minecraft::HandshakePacket>(handler.username));
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
  while(std::chrono::steady_clock::now() < deadline && !handler.disconnected) {
    connection.tick();
    if(handler.chunkDataPackets.load() >= 9) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  connection.disconnect();
  server.stopAndJoin();
  EXPECT_TRUE(handler.loggedIn.load());
  EXPECT_FALSE(handler.disconnected.load()) << handler.disconnectReason;
  EXPECT_GT(handler.teleports.load(), 0);
  EXPECT_GT(handler.chunkStatusLoads.load(), 0);
  EXPECT_GE(handler.chunkDataPackets.load(), 9);
}
TEST(MultiplayerChunkDelivery, IdleSocketClientStaysConnectedAfterInitialChunkBurst) {
  const std::filesystem::path appDataRoot = makeTempAppDataRoot("mp_keepalive_appdata");
#ifdef _WIN32
  _putenv_s("APPDATA", appDataRoot.string().c_str());
  _putenv_s("USERPROFILE", appDataRoot.string().c_str());
#endif
  net::minecraft::mod::runtime::host().shutdown();
  net::minecraft::server::host::ServerLaunchConfig config;
  config.storageRoot = makeTempWorldRoot("mp_keepalive");
  config.worldName = "IdleConnectionWorld";
  config.worldSeed = 123456789ULL;
  config.bindAddress = "127.0.0.1";
  config.port = 0;
  config.onlineMode = false;
  config.useConsoleThread = false;
  config.useGui = false;
  net::minecraft::server::MinecraftServer server(config);
  ASSERT_TRUE(server.startAsync()) << (server.lastError().empty() ? "server failed to start" : server.lastError());
  const std::uint16_t port = server.boundPort();
  ASSERT_NE(port, 0);
  const SOCKET socket = connectLoopback(port);
  ASSERT_NE(socket, INVALID_SOCKET);
  ProbeClientHandler handler;
  Connection connection(socket, "Probe", handler);
  handler.connection = &connection;
  connection.sendPacket(std::make_unique<net::minecraft::HandshakePacket>(handler.username));
  const auto loginDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
  while(std::chrono::steady_clock::now() < loginDeadline && !handler.disconnected) {
    connection.tick();
    if(handler.loggedIn.load() && handler.chunkDataPackets.load() >= 9) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  ASSERT_TRUE(handler.loggedIn.load());
  ASSERT_FALSE(handler.disconnected.load());
  const auto idleDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
  while(std::chrono::steady_clock::now() < idleDeadline && !handler.disconnected) {
    connection.tick();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  const int keepAlivesSeen = handler.keepAlives.load();
  connection.disconnect();
  server.stopAndJoin();
  EXPECT_FALSE(handler.disconnected.load()) << handler.disconnectReason;
  EXPECT_GT(keepAlivesSeen, 0) << "server should send keep-alives after the initial chunk burst";
}
} // namespace net::minecraft::test
