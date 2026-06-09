#pragma once

#include "net/minecraft/network/Connection.hpp"

#include <memory>
#include <type_traits>

namespace net::minecraft::client::network {

class ClientPacketSender {
public:
    ClientPacketSender() = default;
    explicit ClientPacketSender(Connection* connection) noexcept;

    void bind(Connection* connection) noexcept;

    template <typename PacketT>
    void send(const PacketT& packet)
    {
        if (connection_ != nullptr && connection_->isOpen()) {
            connection_->sendPacket<PacketT>(packet);
        }
    }

private:
    Connection* connection_ = nullptr;
};

} // namespace net::minecraft::client::network
