#pragma once
#include <atomic>
#include <memory>
#include <string>
namespace net::minecraft {
class Connection;
}
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::core {
class WorldSession;
}
namespace net::minecraft::client::multiplayer {
class ClientNetworkHandler;
}
namespace net::minecraft::client::multiplayer {
class ClientNetworkBridge {
public:
  explicit ClientNetworkBridge(core::WorldSession* worldSession) noexcept;
  // Out-of-line: handler_/connection_ are unique_ptrs to forward-declared types, so the
  // destructor must be emitted in the .cpp where those types are complete.
  ~ClientNetworkBridge();
  bool connect(client::Minecraft* minecraft,
               const std::string& host,
               int port,
               std::string& errorOut,
               const std::atomic_bool* canceled = nullptr);
  void disconnect(const std::string& reason = "Disconnected");
  void tick();
  [[nodiscard]] multiplayer::ClientNetworkHandler* handler() const noexcept;
  [[nodiscard]] net::minecraft::Connection* connection() const noexcept;

private:
  core::WorldSession* worldSession_ = nullptr;
  std::unique_ptr<net::minecraft::Connection> connection_;
  std::unique_ptr<multiplayer::ClientNetworkHandler> handler_;
};
} // namespace net::minecraft::client::multiplayer
