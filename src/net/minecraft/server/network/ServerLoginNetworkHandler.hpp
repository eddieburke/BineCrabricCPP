#pragma once
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/packet/ConnectionPackets.hpp"
#include "net/minecraft/server/network/ServerSocket.hpp"
namespace net::minecraft {
class Connection;
class LuaModSyncPacket;
} // namespace net::minecraft
namespace net::minecraft::server {
class MinecraftServer;
}
namespace net::minecraft::server::network {
class ConnectionListener;
class ServerLoginNetworkHandler : public NetworkHandler {
 public:
 ServerLoginNetworkHandler(MinecraftServer* server,
                           ConnectionListener* listener,
                           SOCKET socket,
                           std::string connectionName,
                           bool onlineMode);
 ~ServerLoginNetworkHandler() override;
 ServerLoginNetworkHandler(const ServerLoginNetworkHandler&) = delete;
 ServerLoginNetworkHandler& operator=(const ServerLoginNetworkHandler&) = delete;
 void tick();
 void disconnect(const std::string& reason);
 [[nodiscard]] Connection* connection() const noexcept;
 [[nodiscard]] bool isServerSide() const override {
  return true;
 }
 void onHandshake(const HandshakePacket& packet) override;
 void onHello(const LoginHelloPacket& packet) override;
 void onLuaModSync(const LuaModSyncPacket& packet) override;
 void handle(const Packet& packet) override;
 void onDisconnected(const std::string& reason, const std::vector<std::string>& args) override;
 [[nodiscard]] std::string getConnectionInfo() const;
 bool closed = false;

 private:
 void accept(const LoginHelloPacket& packet);
 void verifyUsernameOnline(const LoginHelloPacket& packet);
 [[nodiscard]] std::vector<std::string> requiredWorldMods() const;
 [[nodiscard]] static std::string urlEncodeComponent(const std::string& value);
 MinecraftServer* server_ = nullptr;
 ConnectionListener* listener_ = nullptr;
 bool onlineMode_ = false;
 std::unique_ptr<Connection> connection_;
 int loginTicks_ = 0;
 std::string username_;
 std::optional<LoginHelloPacket> deferredLoginPacket_;
 std::string serverId_;
 std::string clientModsCsv_;
 std::thread verifyThread_;
 std::mutex verifyMutex_;
};
} // namespace net::minecraft::server::network
