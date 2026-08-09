#pragma once
#include <atomic>
#include <memory>
#include <string>
namespace net::minecraft {
class Connection;
class NetworkHandler;
}
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::multiplayer {
class ClientNetworkHandler;
}
namespace net::minecraft::client::multiplayer {
class ClientNetworkBridge {
 public:
 ClientNetworkBridge() noexcept;
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
 [[nodiscard]] net::minecraft::NetworkHandler* handler() const noexcept;
 [[nodiscard]] net::minecraft::Connection* connection() const noexcept;
 void setHandler(std::unique_ptr<net::minecraft::NetworkHandler> newHandler);

 private:
 std::unique_ptr<net::minecraft::Connection> connection_;
 std::unique_ptr<net::minecraft::NetworkHandler> handler_;
 std::unique_ptr<net::minecraft::NetworkHandler> retiredHandler_;
};
} // namespace net::minecraft::client::multiplayer
