#include "net/minecraft/client/network/MultiplayerClientPlayerEntity.hpp"

#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/util/Session.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/network/packet/ChatPackets.hpp"
#include "net/minecraft/network/packet/EntityPackets.hpp"
#include "net/minecraft/network/packet/PlayerPackets.hpp"
#include "net/minecraft/network/packet/WorldPackets.hpp"
#include "net/minecraft/stat/Stats.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

#include <utility>

namespace net::minecraft::client::network {
namespace {

constexpr int kClientCommandPressShift = 1;
constexpr int kClientCommandReleaseShift = 2;

client::option::GameOptions& defaultOptions()
{
    static client::option::GameOptions options;
    return options;
}

} // namespace

MultiplayerClientPlayerEntity::MultiplayerClientPlayerEntity(
    client::Minecraft* minecraft,
    World* world,
    const client::util::Session& session,
    ClientNetworkHandler* networkHandler)
    : entity::player::ClientPlayerEntity(minecraft, world, session, defaultOptions(), 0),
      networkHandler(networkHandler)
{
}

bool MultiplayerClientPlayerEntity::damage(Entity* /*damageSource*/, int /*amount*/)
{
    return false;
}

void MultiplayerClientPlayerEntity::heal(int /*amount*/)
{
}

void MultiplayerClientPlayerEntity::tick()
{
    if (world == nullptr) {
        return;
    }

    if (!world->isPosLoaded(MathHelper::floor(x), 64, MathHelper::floor(z))) {
        return;
    }

    entity::player::ClientPlayerEntity::tick();
    sendMovementPackets();
}

void MultiplayerClientPlayerEntity::sendMovementPackets()
{
    if (packetSendCounter++ == 20) {
        syncStateBeforeRespawn();
        packetSendCounter = 0;
    }

    const bool sneaking = isSneaking();
    if (sneaking != lastSneaking) {
        if (networkHandler != nullptr) {
            ClientCommandC2SPacket packet;
            packet.entityId = id;
            packet.mode = sneaking ? kClientCommandPressShift : kClientCommandReleaseShift;
            networkHandler->sendPacket(packet);
        }
        lastSneaking = sneaking;
    }

    const double deltaX = x - lastSentX;
    const double deltaBbMinY = boundingBox.minY - lastSentBbMinY;
    const double deltaY = y - lastSentY;
    const double deltaZ = z - lastSentZ;
    const double deltaYaw = yaw - lastSentYaw;
    const double deltaPitch = pitch - lastSentPitch;
    bool moved = deltaX != 0.0 || deltaBbMinY != 0.0 || deltaY != 0.0 || deltaZ != 0.0;
    const bool rotated = deltaYaw != 0.0 || deltaPitch != 0.0;

    if (vehicle != nullptr) {
        if (networkHandler != nullptr) {
            if (rotated) {
                PlayerMovePositionAndOnGroundPacket packet;
                packet.x = velocityX;
                packet.y = -999.0;
                packet.eyeHeight = -999.0;
                packet.z = velocityZ;
                packet.onGround = onGround;
                networkHandler->sendPacket(packet);
            } else {
                PlayerMoveFullPacket packet;
                packet.x = velocityX;
                packet.y = -999.0;
                packet.eyeHeight = -999.0;
                packet.z = velocityZ;
                packet.yaw = yaw;
                packet.pitch = pitch;
                packet.onGround = onGround;
                networkHandler->sendPacket(packet);
            }
        }
        moved = false;
    } else if (moved && rotated) {
        if (networkHandler != nullptr) {
            PlayerMoveFullPacket packet;
            packet.x = x;
            packet.y = boundingBox.minY;
            packet.eyeHeight = y;
            packet.z = z;
            packet.yaw = yaw;
            packet.pitch = pitch;
            packet.onGround = onGround;
            networkHandler->sendPacket(packet);
        }
        onGroundTicks = 0;
    } else if (moved) {
        if (networkHandler != nullptr) {
            PlayerMovePositionAndOnGroundPacket packet;
            packet.x = x;
            packet.y = boundingBox.minY;
            packet.eyeHeight = y;
            packet.z = z;
            packet.onGround = onGround;
            networkHandler->sendPacket(packet);
        }
        onGroundTicks = 0;
    } else if (rotated) {
        if (networkHandler != nullptr) {
            PlayerMoveLookAndOnGroundPacket packet;
            packet.yaw = yaw;
            packet.pitch = pitch;
            packet.onGround = onGround;
            networkHandler->sendPacket(packet);
        }
        onGroundTicks = 0;
    } else {
        if (networkHandler != nullptr) {
            PlayerMovePacket packet;
            packet.onGround = onGround;
            networkHandler->sendPacket(packet);
        }
        onGroundTicks = (lastOnGround != onGround || onGroundTicks > 200) ? 0 : onGroundTicks + 1;
    }

    lastOnGround = onGround;
    if (moved) {
        lastSentX = x;
        lastSentBbMinY = boundingBox.minY;
        lastSentY = y;
        lastSentZ = z;
    }
    if (rotated) {
        lastSentYaw = yaw;
        lastSentPitch = pitch;
    }
}

void MultiplayerClientPlayerEntity::dropSelectedItem()
{
    if (networkHandler != nullptr) {
        PlayerActionC2SPacket packet;
        packet.action = 4;
        networkHandler->sendPacket(packet);
    }
}

void MultiplayerClientPlayerEntity::syncStateBeforeRespawn()
{
}

void MultiplayerClientPlayerEntity::spawnItem(ItemEntity* /*itemEntity*/)
{
}

void MultiplayerClientPlayerEntity::sendChatMessage(const std::string& message)
{
    if (networkHandler != nullptr) {
        ChatMessagePacket packet;
        packet.chatMessage = message;
        networkHandler->sendPacket(packet);
    }
}

void MultiplayerClientPlayerEntity::swingHand()
{
    entity::player::ClientPlayerEntity::swingHand();
    if (networkHandler != nullptr) {
        EntityAnimationPacket packet;
        packet.id = id;
        packet.animationId = 1;
        networkHandler->sendPacket(packet);
    }
}

void MultiplayerClientPlayerEntity::respawn()
{
    syncStateBeforeRespawn();
    if (networkHandler != nullptr) {
        PlayerRespawnPacket packet;
        packet.dimensionRawId = static_cast<std::int8_t>(dimensionId);
        networkHandler->sendPacket(packet);
    }
}

void MultiplayerClientPlayerEntity::applyDamage(int amount)
{
    health -= amount;
}

void MultiplayerClientPlayerEntity::closeHandledScreen()
{
    inventory.setCursorStack({});
    entity::player::ClientPlayerEntity::closeHandledScreen();
}

void MultiplayerClientPlayerEntity::damageTo(int healthIn)
{
    health = healthIn;
    prevJumping = true;
}

void MultiplayerClientPlayerEntity::increaseStat(int stat, int amount)
{
    if (stat::Stats::isLocalOnly(stat)) {
        entity::player::ClientPlayerEntity::increaseStat(stat, amount);
    }
}

void MultiplayerClientPlayerEntity::handleIncreaseStat(int stat, int amount)
{
    if (!stat::Stats::isLocalOnly(stat)) {
        entity::player::ClientPlayerEntity::increaseStat(stat, amount);
    }
}

} // namespace net::minecraft::client::network
