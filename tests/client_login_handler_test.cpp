#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <gtest/gtest.h>

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkBridge.hpp"
#include "net/minecraft/client/multiplayer/ClientLoginNetworkHandler.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/packet/ConnectionPackets.hpp"
#include "net/minecraft/server/network/ServerSocket.hpp"
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
#include "net/minecraft/world/ClientWorld.hpp"

namespace {
class MockServerBootstrapHandler : public net::minecraft::NetworkHandler {
 public:
  net::minecraft::Connection* connection = nullptr;
  bool receivedHandshake = false;
  bool receivedHello = false;
  std::string handshakeUsername;
  int helloProtocol = 0;
  std::string helloUsername;
  std::uint64_t helloSeed = 0;
  int helloDimension = 0;

  [[nodiscard]] bool isServerSide() const override {
    return true;
  }

  void onHandshake(const net::minecraft::HandshakePacket& packet) override {
    receivedHandshake = true;
    handshakeUsername = packet.name;
    connection->sendPacket(std::make_unique<net::minecraft::HandshakePacket>("-"));
  }

  void onHello(const net::minecraft::LoginHelloPacket& packet) override {
    receivedHello = true;
    helloProtocol = packet.protocolVersion;
    helloUsername = packet.username;
    helloSeed = packet.worldSeed;
    helloDimension = packet.dimensionId;
  }
};

class TestMinecraft final : public net::minecraft::client::Minecraft {
 public:
  using net::minecraft::client::Minecraft::Minecraft;
  void handleCrash(const net::minecraft::util::crash::CrashReport&) override {}
};

} // namespace

namespace net::minecraft::test {
TEST(ClientLoginHandlerTest, ParityHandshakeSequence) {
  TestMinecraft client(nullptr, nullptr, nullptr, 854, 480, false);
  client.session = client::util::Session{"Player", ""};
  client.options.modsEnabled = false;

  server::network::ServerSocket listenSocket;
  listenSocket.bindAndListen("127.0.0.1", 0);

  auto bridge = std::make_unique<client::multiplayer::ClientNetworkBridge>();
  std::string connectError;
  ASSERT_TRUE(bridge->connect(&client, "127.0.0.1", listenSocket.boundPort(), connectError)) << connectError;

  std::string remoteAddress;
  SOCKET serverSocket = INVALID_SOCKET;
  const auto acceptDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while(std::chrono::steady_clock::now() < acceptDeadline && serverSocket == INVALID_SOCKET) {
    serverSocket = listenSocket.accept(remoteAddress);
    if(serverSocket == INVALID_SOCKET) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  ASSERT_NE(serverSocket, INVALID_SOCKET);
  client::multiplayer::ClientNetworkBridge* liveBridge = bridge.get();
  client.multiplayerSession().adoptBridge(std::move(bridge));

  MockServerBootstrapHandler serverHandler;
  Connection serverConnection(serverSocket, "Server", serverHandler);
  serverHandler.connection = &serverConnection;

  liveBridge->connection()->sendPacket(std::make_unique<HandshakePacket>("Player"));
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (std::chrono::steady_clock::now() < deadline) {
    serverConnection.tick();
    client.multiplayerSession().tick();
    if (serverHandler.receivedHello) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  ASSERT_TRUE(serverHandler.receivedHandshake);
  EXPECT_EQ(serverHandler.handshakeUsername, "Player");
  ASSERT_TRUE(serverHandler.receivedHello);
  EXPECT_EQ(serverHandler.helloUsername, "Player");
  EXPECT_EQ(serverHandler.helloProtocol, kProtocolVersionBeta173);
  EXPECT_EQ(serverHandler.helloSeed, 0ULL);
  EXPECT_EQ(serverHandler.helloDimension, 0);

  LoginHelloPacket serverReply;
  serverReply.protocolVersion = 42;
  serverReply.username = "";
  serverReply.worldSeed = 12345ULL;
  serverReply.dimensionId = 0;
  
  serverConnection.sendPacket(std::make_unique<LoginHelloPacket>(serverReply));

  deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (std::chrono::steady_clock::now() < deadline) {
    client.multiplayerSession().tick();
    if (dynamic_cast<client::multiplayer::ClientNetworkHandler*>(liveBridge->handler()) != nullptr) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  ASSERT_NE(dynamic_cast<client::multiplayer::ClientNetworkHandler*>(liveBridge->handler()), nullptr);
  ASSERT_NE(client.world, nullptr);
  EXPECT_TRUE(client.world->isRemote());
  EXPECT_EQ(client.player->id, 42);

  liveBridge->disconnect();
  serverConnection.disconnect();
}
} // namespace net::minecraft::test
