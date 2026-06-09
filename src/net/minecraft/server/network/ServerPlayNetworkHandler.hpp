#pragma once

#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/NetworkHandler.hpp"

#include <memory>
#include <string>

namespace net::minecraft {

class ClientCommandC2SPacket;
class PlayerInputC2SPacket;
class PlayerMovePacket;

namespace entity::player {
class ServerPlayerEntity;
}

namespace server {

class MinecraftServer;

namespace network {

class ServerPlayNetworkHandler : public NetworkHandler {
public:
    ServerPlayNetworkHandler(MinecraftServer* server, Connection* connection, entity::player::ServerPlayerEntity* player);

    void tick();

    void disconnect(const std::string& reason);

    template <typename PacketT>
    void sendPacket(const PacketT& packet)
    {
        if (connection_ != nullptr) {
            connection_->sendPacket(std::make_unique<PacketT>(packet));
            lastKeepAliveTime_ = ticks_;
        }
    }
    void sendPacket(std::unique_ptr<Packet> packet)
    {
        if (connection_ != nullptr && packet) {
            connection_->sendPacket(std::move(packet));
            lastKeepAliveTime_ = ticks_;
        }
    }

    void teleport(double x, double y, double z, float yaw, float pitch);

    [[nodiscard]] std::size_t getBlockDataSendQueueSize() const;
    [[nodiscard]] bool isServerSide() const override { return true; }

    void onPlayerInput(const PlayerInputC2SPacket& packet) override;
    void onPlayerMove(const PlayerMovePacket& packet) override;
    void handleClientCommand(const ClientCommandC2SPacket& packet) override;

private:
    MinecraftServer* server_ = nullptr;
    Connection* connection_ = nullptr;
    entity::player::ServerPlayerEntity* player_ = nullptr;
    int ticks_ = 0;
    int lastKeepAliveTime_ = 0;
    int floatingTime_ = 0;
    bool moved_ = false;
    double teleportTargetX_ = 0.0;
    double teleportTargetY_ = 0.0;
    double teleportTargetZ_ = 0.0;
    bool teleported_ = true;
};

} // namespace network
} // namespace server
} // namespace net::minecraft
