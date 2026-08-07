#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <gtest/gtest.h>

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/multiplayer/ClientLoginNetworkHandler.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/packet/ConnectionPackets.hpp"
#include "net/minecraft/server/network/ServerSocket.hpp"
#include "net/minecraft/server/network/Socket.hpp"
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"

namespace {
class MockServerBootstrapHandler : public net::minecraft::NetworkHandler {
 public:
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

SOCKET connectLoopback(int port) {
  SOCKET s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = htons(static_cast<std::uint16_t>(port));
  ::connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
  return s;
}
} // namespace

namespace net::minecraft::test {
TEST(ClientLoginHandlerTest, ParityHandshakeSequence) {
  TestMinecraft client(nullptr, nullptr, nullptr, 854, 480, false);
  client::multiplayer::ClientLoginNetworkHandler clientHandler(&client);
  client.session = client::util::Session{"Player", "", "token", ""};
  client.options.modsEnabled = false;

  server::network::ServerSocket listenSocket;
  listenSocket.bindAndListen("127.0.0.1", 0);

  SOCKET clientSocket = INVALID_SOCKET;
  std::thread connector([&]() { clientSocket = connectLoopback(listenSocket.boundPort()); });

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

  Connection clientConnection(clientSocket, "Client", clientHandler);
  clientHandler.bindConnection(&clientConnection);

  MockServerBootstrapHandler serverHandler;
  Connection serverConnection(serverSocket, "Server", serverHandler);

  // 1. Client connects to server and receives Handshake "-" (offline mode)
  clientHandler.onHandshake(HandshakePacket{"-"});
  
  // Tick client handler to process joinServerWork_
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (std::chrono::steady_clock::now() < deadline) {
    clientHandler.tick();
    serverConnection.tick();
    if (serverHandler.receivedHello) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // 2. Client must send LoginHelloPacket with the exact MCP structure
  ASSERT_TRUE(serverHandler.receivedHello);
  EXPECT_EQ(serverHandler.helloUsername, "Player");
  EXPECT_EQ(serverHandler.helloProtocol, kProtocolVersionBeta173);
  EXPECT_EQ(serverHandler.helloSeed, 0ULL);
  EXPECT_EQ(serverHandler.helloDimension, 0);

  // 3. Server replies with LoginHelloPacket (entityId, seed, dimension)
  LoginHelloPacket serverReply;
  serverReply.protocolVersion = 42; // Entity ID in MCP protocol
  serverReply.username = ""; // Empty in MCP Server->Client response
  serverReply.worldSeed = 12345ULL;
  serverReply.dimensionId = 0;
  
  serverConnection.sendPacket(std::make_unique<LoginHelloPacket>(serverReply));

  deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (std::chrono::steady_clock::now() < deadline) {
    clientConnection.tick();
    if (dynamic_cast<client::multiplayer::ClientNetworkHandler*>(clientConnection.networkHandler()) != nullptr) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // 4. Verify client transitioned to Play handler successfully
  ASSERT_NE(dynamic_cast<client::multiplayer::ClientNetworkHandler*>(clientConnection.networkHandler()), nullptr);

  clientConnection.disconnect();
  serverConnection.disconnect();
}
} // namespace net::minecraft::test
