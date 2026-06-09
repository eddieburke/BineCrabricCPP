#include "net/minecraft/client/network/ClientPacketSender.hpp"

namespace net::minecraft::client::network {

ClientPacketSender::ClientPacketSender(Connection* connection) noexcept
    : connection_(connection)
{
}

void ClientPacketSender::bind(Connection* connection) noexcept
{
    connection_ = connection;
}

} // namespace net::minecraft::client::network
