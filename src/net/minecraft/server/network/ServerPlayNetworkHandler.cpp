#include "net/minecraft/server/network/ServerPlayNetworkHandler.hpp"

#include "net/minecraft/entity/player/ServerPlayerEntity.hpp"
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/packet/ConnectionPackets.hpp"
#include "net/minecraft/network/packet/PlayerPackets.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/PlayerManager.hpp"
#include "net/minecraft/world/ServerWorld.hpp"

#include <cmath>
#include <iostream>

namespace net::minecraft::server::network {

namespace {

constexpr int kClientCommandPressShift = 1;
constexpr int kClientCommandReleaseShift = 2;
constexpr int kClientCommandStopSleeping = 3;

} // namespace

ServerPlayNetworkHandler::ServerPlayNetworkHandler(
    MinecraftServer* server,
    Connection* connection,
    entity::player::ServerPlayerEntity* player)
    : server_(server),
      connection_(connection),
      player_(player)
{
    if (player_ != nullptr) {
        player_->networkHandler = this;
        teleportTargetX_ = player_->x;
        teleportTargetY_ = player_->y;
        teleportTargetZ_ = player_->z;
    }
    if (connection_ != nullptr) {
        connection_->setNetworkHandler(*this);
    }
}

void ServerPlayNetworkHandler::tick()
{
    moved_ = false;
    if (connection_ != nullptr) {
        connection_->tick();
    }
    if (ticks_ - lastKeepAliveTime_ > 20) {
        sendPacket(KeepAlivePacket{});
    }
    ++ticks_;
}

std::size_t ServerPlayNetworkHandler::getBlockDataSendQueueSize() const
{
    if (connection_ == nullptr) {
        return 0;
    }
    return connection_->getDelayedSendQueueSize();
}

void ServerPlayNetworkHandler::disconnect(const std::string& reason)
{
    if (connection_ != nullptr) {
        DisconnectPacket packet;
        packet.reason = reason;
        sendPacket(packet);
        connection_->disconnect();
    }
}

void ServerPlayNetworkHandler::onPlayerInput(const PlayerInputC2SPacket& packet)
{
    if (player_ == nullptr) {
        return;
    }
    player_->updateInput(
        packet.sideways,
        packet.forward,
        packet.jumping,
        packet.sneaking,
        packet.pitch,
        packet.yaw);
}

void ServerPlayNetworkHandler::handleClientCommand(const ClientCommandC2SPacket& packet)
{
    if (player_ == nullptr) {
        return;
    }
    if (packet.mode == kClientCommandPressShift) {
        player_->setSneaking(true);
    } else if (packet.mode == kClientCommandReleaseShift) {
        player_->setSneaking(false);
    } else if (packet.mode == kClientCommandStopSleeping) {
        player_->wakeUp(false, true, true);
        teleported_ = false;
    }
}

void ServerPlayNetworkHandler::teleport(double x, double y, double z, float yaw, float pitch)
{
    if (player_ == nullptr) {
        return;
    }
    teleported_ = false;
    teleportTargetX_ = x;
    teleportTargetY_ = y;
    teleportTargetZ_ = z;
    player_->setPositionAndAngles(x, y, z, yaw, pitch);
    PlayerMoveFullPacket packet;
    packet.x = x;
    packet.y = y + static_cast<double>(player_->getEyeHeight());
    packet.eyeHeight = y;
    packet.z = z;
    packet.yaw = yaw;
    packet.pitch = pitch;
    packet.onGround = false;
    sendPacket(packet);
}

void ServerPlayNetworkHandler::onPlayerMove(const PlayerMovePacket& packet)
{
    if (player_ == nullptr || server_ == nullptr) {
        return;
    }

    ServerWorld* serverWorld = server_->getWorld(player_->dimensionId);
    if (serverWorld == nullptr) {
        return;
    }

    moved_ = true;
    const double startY = player_->y;

    if (!teleported_) {
        const double deltaToTargetY = packet.y - teleportTargetY_;
        if (packet.changePosition && packet.x == teleportTargetX_
            && deltaToTargetY * deltaToTargetY < 0.01 && packet.z == teleportTargetZ_) {
            teleported_ = true;
        }
    }

    if (!teleported_) {
        return;
    }

    if (player_->vehicle != nullptr) {
        float yaw = player_->yaw;
        float pitch = player_->pitch;
        player_->vehicle->updatePassengerPosition();
        const double savedX = player_->x;
        const double savedY = player_->y;
        const double savedZ = player_->z;
        double moveX = 0.0;
        double moveZ = 0.0;
        if (packet.changeLook) {
            yaw = packet.yaw;
            pitch = packet.pitch;
        }
        if (packet.changePosition && packet.y == -999.0 && packet.eyeHeight == -999.0) {
            moveX = packet.x;
            moveZ = packet.z;
        }
        player_->onGround = packet.onGround;
        player_->playerTick(true);
        player_->move(moveX, 0.0, moveZ);
        player_->setPositionAndAngles(savedX, savedY, savedZ, yaw, pitch);
        player_->velocityX = moveX;
        player_->velocityZ = moveZ;
        if (player_->vehicle != nullptr) {
            serverWorld->tickVehicle(player_->vehicle, true);
            player_->vehicle->updatePassengerPosition();
        }
        if (server_ != nullptr) {
            server_->playerManager.updatePlayerChunks(player_);
        }
        teleportTargetX_ = player_->x;
        teleportTargetY_ = player_->y;
        teleportTargetZ_ = player_->z;
        serverWorld->updateEntity(player_, true);
        return;
    }

    if (player_->isSleeping()) {
        player_->playerTick(true);
        player_->setPositionAndAngles(teleportTargetX_, teleportTargetY_, teleportTargetZ_, player_->yaw, player_->pitch);
        serverWorld->updateEntity(player_, true);
        return;
    }

    teleportTargetX_ = player_->x;
    teleportTargetY_ = player_->y;
    teleportTargetZ_ = player_->z;

    double targetX = player_->x;
    double targetY = player_->y;
    double targetZ = player_->z;
    float targetYaw = player_->yaw;
    float targetPitch = player_->pitch;

    PlayerMovePacket workingPacket = packet;
    if (workingPacket.changePosition && workingPacket.y == -999.0 && workingPacket.eyeHeight == -999.0) {
        workingPacket.changePosition = false;
    }

    if (workingPacket.changePosition) {
        targetX = workingPacket.x;
        targetY = workingPacket.y;
        targetZ = workingPacket.z;
        const double stanceDelta = workingPacket.eyeHeight - workingPacket.y;
        if (!player_->isSleeping() && (stanceDelta > 1.65 || stanceDelta < 0.1)) {
            disconnect("Illegal stance");
            return;
        }
        if (std::abs(workingPacket.x) > 3.2E7 || std::abs(workingPacket.z) > 3.2E7) {
            disconnect("Illegal position");
            return;
        }
    }

    if (workingPacket.changeLook) {
        targetYaw = workingPacket.yaw;
        targetPitch = workingPacket.pitch;
    }

    player_->playerTick(true);
    player_->cameraOffset = 0.0f;
    player_->setPositionAndAngles(teleportTargetX_, teleportTargetY_, teleportTargetZ_, targetYaw, targetPitch);

    double deltaX = targetX - player_->x;
    double deltaY = targetY - player_->y;
    double deltaZ = targetZ - player_->z;
    const double distanceSq = deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ;
    if (distanceSq > 100.0) {
        disconnect("You moved too quickly :( (Hacking?)");
        return;
    }

    constexpr float collisionMargin = 0.0625f;
    const bool hadSpaceBeforeMove =
        serverWorld->getEntityCollisions(player_, player_->boundingBox.contract(collisionMargin)).empty();

    player_->move(deltaX, deltaY, deltaZ);

    deltaX = targetX - player_->x;
    deltaY = targetY - player_->y;
    if (deltaY > -0.5 || deltaY < 0.5) {
        deltaY = 0.0;
    }
    deltaZ = targetZ - player_->z;
    const double afterMoveDistanceSq = deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ;

    bool movedWrongly = false;
    if (afterMoveDistanceSq > 0.0625 && !player_->isSleeping()) {
        movedWrongly = true;
        std::cout << player_->name << " moved wrongly!" << std::endl;
    }

    player_->setPositionAndAngles(targetX, targetY, targetZ, targetYaw, targetPitch);
    const bool hasSpaceAfterMove =
        serverWorld->getEntityCollisions(player_, player_->boundingBox.contract(collisionMargin)).empty();
    if (hadSpaceBeforeMove && (movedWrongly || !hasSpaceAfterMove) && !player_->isSleeping()) {
        teleport(teleportTargetX_, teleportTargetY_, teleportTargetZ_, targetYaw, targetPitch);
        return;
    }

    if (deltaY >= -0.03125 && !packet.onGround) {
        ++floatingTime_;
    } else {
        floatingTime_ = 0;
    }
    if (!server_->flightEnabled && floatingTime_ > 80) {
        disconnect("Flying is not enabled on this server");
        return;
    }

    player_->onGround = packet.onGround;
    if (server_ != nullptr) {
        server_->playerManager.updatePlayerChunks(player_);
    }
    player_->handleFall(player_->y - startY, packet.onGround);
}

} // namespace net::minecraft::server::network
