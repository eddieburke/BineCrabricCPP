#include "net/minecraft/client/multiplayer/MultiplayerConnector.hpp"
#include "net/minecraft/client/auth/microsoft/SessionRestore.hpp"
#include "net/minecraft/client/session/OfflineIdentity.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/multiplayer/MultiplayerSession.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/packet/ConnectionPackets.hpp"
#include <utility>
namespace net::minecraft::client::multiplayer {
MultiplayerConnector::MultiplayerConnector(Minecraft* minecraft, std::string host, int port, ConnectOptions options)
    : options_(options) {
  if(minecraft == nullptr) {
    return;
  }
  thread_ = std::thread([this, minecraft, host = std::move(host), port]() {
    if(cancelled_.load(std::memory_order_acquire)) {
      return;
    }
    std::string connectError;
    auto bridge = std::make_unique<ClientNetworkBridge>(&minecraft->worldSession());
    if(!bridge->connect(minecraft, host, port, connectError, &cancelled_)) {
      if(cancelled_.load(std::memory_order_acquire)) {
        return;
      }
      std::lock_guard lock(mutex_);
      connectError_ = connectError.empty() ? "Failed to connect" : std::move(connectError);
      return;
    }
    if(cancelled_.load(std::memory_order_acquire)) {
      bridge->disconnect();
      return;
    }
    // Restore the saved Microsoft account before the first handshake: the server locks in
    // the name we send here, so a not-yet-applied async restore would join under the launch
    // fallback ("PlayerNNN") instead of the real profile.
    // Offline LAN handoff already runs on loopback with online-mode disabled; skip the
    // blocking Microsoft refresh so the host can join immediately.
    if(!options_.lanLoopbackHandoff &&
       !::msauth::ensureAuthenticatedForJoin(*minecraft, &cancelled_)) {
      if(cancelled_.load(std::memory_order_acquire)) {
        bridge->disconnect();
        return;
      }
      bridge->disconnect();
      std::lock_guard lock(mutex_);
      if(const std::optional<std::string> restoreError = ::msauth::lastSavedAccountRestoreError()) {
        connectError_ = *restoreError;
      } else {
        connectError_ = "Could not sign in to your Microsoft account. Sign in again from the title screen.";
      }
      return;
    }
    if(net::minecraft::Connection* connection = bridge->connection()) {
      HandshakePacket handshake{::net::minecraft::client::session::resolveJoinUsername(minecraft->session)};
      connection->sendPacket(std::make_unique<HandshakePacket>(std::move(handshake)));
    }
    if(multiplayer::ClientNetworkHandler* handler = bridge->handler()) {
      handler->message = resource::language::I18n::getTranslation("connect.authorizing");
    }
    std::lock_guard lock(mutex_);
    pendingBridge_ = std::move(bridge);
  });
}
MultiplayerConnector::~MultiplayerConnector() {
  cancel();
  if(thread_.joinable()) {
    thread_.join();
  }
}
void MultiplayerConnector::cancel() {
  cancelled_.store(true, std::memory_order_release);
}
void MultiplayerConnector::disconnectActive(Minecraft& client) {
  cancelled_.store(true, std::memory_order_release);
  ClientNetworkBridge* bridge = nullptr;
  {
    std::lock_guard lock(mutex_);
    bridge = pendingBridge_ != nullptr ? pendingBridge_.get() : client.multiplayerSession().bridge();
  }
  if(bridge != nullptr) {
    bridge->disconnect();
  }
}
std::string MultiplayerConnector::poll(Minecraft& client) {
  std::lock_guard lock(mutex_);
  if(connectError_.has_value()) {
    std::string error = std::move(*connectError_);
    connectError_.reset();
    return error.empty() ? "Failed to connect" : std::move(error);
  }
  if(pendingBridge_ != nullptr) {
    client.multiplayerSession().adoptBridge(std::move(pendingBridge_));
  }
  return {};
}
void MultiplayerConnector::tickBridge(Minecraft& client) {
  if(cancelled_.load(std::memory_order_acquire)) {
    return;
  }
  if(ClientNetworkBridge* bridge = client.multiplayerSession().bridge()) {
    bridge->tick();
  }
}
ClientNetworkBridge* MultiplayerConnector::activeBridge(Minecraft* client) const {
  if(client == nullptr) {
    return nullptr;
  }
  std::lock_guard lock(mutex_);
  if(pendingBridge_ != nullptr) {
    return pendingBridge_.get();
  }
  return client->multiplayerSession().bridge();
}
} // namespace net::minecraft::client::multiplayer
