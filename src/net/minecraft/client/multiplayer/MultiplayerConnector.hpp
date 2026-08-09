#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include "net/minecraft/client/multiplayer/ClientNetworkBridge.hpp"
#include "net/minecraft/client/util/Session.hpp"
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::multiplayer {
struct ConnectOptions {
 bool bypassAuthentication = false;
};
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
 [[nodiscard]] ClientNetworkBridge* activeBridge(Minecraft* client) const;

 private:
 struct Result {
  std::unique_ptr<ClientNetworkBridge> bridge;
  std::optional<util::Session> authenticatedSession;
  std::optional<std::string> error;
 };
 struct State {
  std::atomic_bool cancelled{false};
  mutable std::mutex mutex;
  std::optional<Result> result;
 };
 std::shared_ptr<State> state_ = std::make_shared<State>();
};
} // namespace net::minecraft::client::multiplayer
