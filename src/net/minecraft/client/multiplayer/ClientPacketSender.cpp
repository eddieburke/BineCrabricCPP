#include "net/minecraft/client/multiplayer/ClientPacketSender.hpp"
namespace net::minecraft::client::multiplayer {
ClientPacketSender::ClientPacketSender(Connection* connection) noexcept : connection_(connection) {}
void ClientPacketSender::bind(Connection* connection) noexcept {
  connection_ = connection;
}
} // namespace net::minecraft::client::multiplayer
