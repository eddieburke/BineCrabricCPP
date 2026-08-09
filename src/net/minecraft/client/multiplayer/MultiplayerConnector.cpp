#include "net/minecraft/client/multiplayer/MultiplayerConnector.hpp"
#include <utility>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/auth/microsoft/SessionRestore.hpp"
#include "net/minecraft/client/session/OfflineIdentity.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include "net/minecraft/client/multiplayer/ClientLoginNetworkHandler.hpp"
#include "net/minecraft/client/multiplayer/MultiplayerSession.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/packet/ConnectionPackets.hpp"
namespace net::minecraft::client::multiplayer {
MultiplayerConnector::MultiplayerConnector(Minecraft* minecraft, std::string host, int port, ConnectOptions options)
    {
 if(minecraft == nullptr) {
  return;
 }
 const auto state = state_;
 const util::Session session = minecraft->session;
 net::minecraft::util::concurrent::ThreadCoordinator::instance()
     .pool(net::minecraft::util::concurrent::Domain::Io)
     .submit([state, minecraft, host = std::move(host), port, options, session]() mutable {
  if(state->cancelled.load(std::memory_order_acquire)) {
   return;
  }
  util::Session authenticatedSession = session;
  if(!options.bypassAuthentication && !::msauth::ensureAuthenticatedForJoin(authenticatedSession, &state->cancelled)) {
   if(state->cancelled.load(std::memory_order_acquire)) {
    return;
   }
   Result result;
   if(const std::optional<std::string> restoreError = ::msauth::lastSavedAccountRestoreError()) {
    result.error = *restoreError;
   } else {
    result.error = "Could not sign in to your Microsoft account. Sign in again from the title screen.";
   }
   std::lock_guard lock(state->mutex);
   state->result = std::move(result);
   return;
  }
  std::string connectError;
  auto bridge = std::make_unique<ClientNetworkBridge>();
  if(!bridge->connect(minecraft, host, port, connectError, &state->cancelled)) {
   if(state->cancelled.load(std::memory_order_acquire)) {
    return;
   }
   Result result;
   result.error = connectError.empty() ? "Failed to connect" : std::move(connectError);
   std::lock_guard lock(state->mutex);
   state->result = std::move(result);
   return;
  }
  if(state->cancelled.load(std::memory_order_acquire)) {
   return;
  }
  Result result;
  result.bridge = std::move(bridge);
  result.authenticatedSession = std::move(authenticatedSession);
  std::lock_guard lock(state->mutex);
  state->result = std::move(result);
 });
}
MultiplayerConnector::~MultiplayerConnector() {
 cancel();
}
void MultiplayerConnector::cancel() {
 state_->cancelled.store(true, std::memory_order_release);
}
void MultiplayerConnector::disconnectActive(Minecraft& client) {
 state_->cancelled.store(true, std::memory_order_release);
 ClientNetworkBridge* bridge = nullptr;
 {
  std::lock_guard lock(state_->mutex);
  bridge = state_->result.has_value() && state_->result->bridge != nullptr ? state_->result->bridge.get()
                                                                            : client.multiplayerSession().bridge();
 }
 if(bridge != nullptr) {
  bridge->disconnect();
 }
}
std::string MultiplayerConnector::poll(Minecraft& client) {
 std::optional<Result> result;
 {
  std::lock_guard lock(state_->mutex);
  result = std::move(state_->result);
  state_->result.reset();
 }
 if(!result.has_value()) {
  return {};
 }
 if(result->error.has_value()) {
  return result->error->empty() ? "Failed to connect" : std::move(*result->error);
 }
 if(result->bridge != nullptr) {
  if(result->authenticatedSession.has_value()) {
   client.session = std::move(*result->authenticatedSession);
  }
  if(net::minecraft::Connection* connection = result->bridge->connection()) {
   HandshakePacket handshake{::net::minecraft::client::session::resolveJoinUsername(client.session)};
   connection->sendPacket(std::make_unique<HandshakePacket>(std::move(handshake)));
  }
  if(auto* handler = dynamic_cast<multiplayer::ClientLoginNetworkHandler*>(result->bridge->handler())) {
   handler->message = resource::language::I18n::getTranslation("connect.authorizing");
  }
  client.multiplayerSession().adoptBridge(std::move(result->bridge));
 }
 return {};
}
ClientNetworkBridge* MultiplayerConnector::activeBridge(Minecraft* client) const {
 if(client == nullptr) {
  return nullptr;
 }
 std::lock_guard lock(state_->mutex);
 if(state_->result.has_value() && state_->result->bridge != nullptr) {
  return state_->result->bridge.get();
 }
 return client->multiplayerSession().bridge();
}
} // namespace net::minecraft::client::multiplayer
