#pragma once
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "net/minecraft/server/network/ServerSocket.hpp"
namespace net::minecraft {
class Connection;
}
namespace net::minecraft::server {
class MinecraftServer;
}
namespace net::minecraft::server::network {
class ServerLoginNetworkHandler;
class ServerPlayNetworkHandler;
class ConnectionListener {
public:
  ConnectionListener(MinecraftServer* server, const std::string& bindAddress, int port, bool onlineMode);
  ~ConnectionListener();
  ConnectionListener(const ConnectionListener&) = delete;
  ConnectionListener& operator=(const ConnectionListener&) = delete;
  void tick();
  void addConnection(std::unique_ptr<ServerPlayNetworkHandler> handler, std::unique_ptr<Connection> connection);
  void stopAccepting();
  void close();
  [[nodiscard]] std::uint16_t boundPort() const;

private:
  struct ActivePlayConnection {
    std::unique_ptr<Connection> connection;
    std::unique_ptr<ServerPlayNetworkHandler> handler;
  };
  void listenLoop();
  void addPendingConnection(std::unique_ptr<ServerLoginNetworkHandler> handler);
  MinecraftServer* server_ = nullptr;
  bool onlineMode_ = false;
  ServerSocket socket_;
  std::thread thread_;
  std::once_flag acceptStopFlag_;
  std::atomic<bool> open_{false};
  int connectionCounter_ = 0;
  std::mutex mutex_;
  std::vector<std::unique_ptr<ServerLoginNetworkHandler>> pendingConnections_;
  std::vector<ActivePlayConnection> playConnections_;
  std::unordered_map<std::string, std::chrono::steady_clock::time_point> recentConnectionsByHost_;
};
} // namespace net::minecraft::server::network
