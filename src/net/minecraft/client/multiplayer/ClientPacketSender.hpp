#pragma once
#include "net/minecraft/network/Connection.hpp"
#include <memory>
#include <type_traits>
namespace net::minecraft::client::multiplayer {
class ClientPacketSender {
public:
  ClientPacketSender() = default;
  explicit ClientPacketSender(Connection* connection) noexcept;
  void bind(Connection* connection) noexcept;
  void send(std::unique_ptr<Packet> packet) {
    if(connection_ != nullptr && connection_->isOpen()) {
      connection_->sendPacket(std::move(packet));
    }
  }
  template <typename PacketT>
  void send(const PacketT& packet) {
    if(connection_ != nullptr && connection_->isOpen()) {
      connection_->sendPacket<PacketT>(packet);
    }
  }

private:
  Connection* connection_ = nullptr;
};
} // namespace net::minecraft::client::multiplayer
