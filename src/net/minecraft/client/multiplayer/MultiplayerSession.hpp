#pragma once
#include <memory>
#include <vector>
namespace net::minecraft::client::multiplayer {
class ClientNetworkBridge;
/// Persistent owner of the live multiplayer connection. Bridges outlive ConnectScreen
/// retirement; teardown is deferred so a handler whose tick() requested disconnect is
/// freed only after that stack unwinds.
class MultiplayerSession {
public:
  MultiplayerSession() = default;
  ~MultiplayerSession();
  void adoptBridge(std::unique_ptr<ClientNetworkBridge> bridge);
  void retireBridge();
  void flushRetired();
  void tick();
  [[nodiscard]] ClientNetworkBridge* bridge() const noexcept {
    return bridge_.get();
  }

private:
  std::unique_ptr<ClientNetworkBridge> bridge_;
  std::vector<std::unique_ptr<ClientNetworkBridge>> retiredBridges_;
};
} // namespace net::minecraft::client::multiplayer
