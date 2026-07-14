#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include "net/minecraft/client/multiplayer/ClientNetworkBridge.hpp"
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::core {
class WorldSession;
}
namespace net::minecraft::client::multiplayer {
struct ConnectOptions {
  bool bypassAuthentication = false;
};
/// Background connect worker. Owns the connect thread and hands the live bridge to
/// MultiplayerSession once the socket is open.
class MultiplayerConnector {
public:
  MultiplayerConnector(Minecraft* minecraft, std::string host, int port, ConnectOptions options = {});
  ~MultiplayerConnector();
  MultiplayerConnector(const MultiplayerConnector&) = delete;
  MultiplayerConnector& operator=(const MultiplayerConnector&) = delete;
  void cancel();
  void disconnectActive(Minecraft& client);
  /// Poll connect progress on the main thread. Returns a non-empty error when connect failed.
  [[nodiscard]] std::string poll(Minecraft& client);
  void tickBridge(Minecraft& client);
  [[nodiscard]] ClientNetworkBridge* activeBridge(Minecraft* client) const;

private:
  ConnectOptions options_;
  std::unique_ptr<ClientNetworkBridge> pendingBridge_;
  std::atomic<bool> cancelled_{false};
  mutable std::mutex mutex_;
  std::optional<std::string> connectError_;
  std::thread thread_;
};
} // namespace net::minecraft::client::multiplayer
