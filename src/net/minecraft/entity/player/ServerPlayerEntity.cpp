#include "net/minecraft/world/World.hpp"

#include "net/minecraft/entity/player/ServerPlayerEntity.hpp"

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/NetworkSyncedItem.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/packet/InventoryPackets.hpp"
#include "net/minecraft/stat/Stats.hpp"

#include "net/minecraft/entity/projectile/ArrowEntity.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/PlayerManager.hpp"
#include "net/minecraft/server/network/ServerPlayNetworkHandler.hpp"

namespace net::minecraft::entity::player {

ServerPlayerEntity::ServerPlayerEntity(
    server::MinecraftServer* serverIn,
    World* world,
    const std::string& nameIn,
    server::network::ServerPlayerInteractionManager* interactionManagerIn)
    : PlayerEntity(world),
      interactionManager(world != nullptr ? world : nullptr)
{
    if (world != nullptr) {
        const Vec3i spawn = world->getSpawnPos();
        int spawnX = spawn.x;
        int spawnZ = spawn.z;
        int spawnY = spawn.y;
        if (world->dimension == nullptr || !world->dimension->hasCeiling) {
            spawnX += random.nextInt(20) - 10;
            spawnZ += random.nextInt(20) - 10;
            spawnY = world->getSpawnPositionValidityY(spawnX, spawnZ);
        }
        setPositionAndAnglesKeepPrevAngles(
            static_cast<double>(spawnX) + 0.5,
            static_cast<double>(spawnY),
            static_cast<double>(spawnZ) + 0.5,
            0.0f,
            0.0f);
    }
    server = serverIn;
    stepHeight = 0.0f;
    name = nameIn;
    standingEyeHeight = 0.0f;
    if (interactionManagerIn != nullptr) {
        interactionManager = *interactionManagerIn;
    }
    interactionManager.world = world;
    interactionManager.player = this;
    lastX = x;
    lastZ = z;
}

ItemStack ServerPlayerEntity::getEquipment(int slot) const
{
    if (slot == 0) {
        const ItemStack* selected = inventory.getSelectedItem();
        return selected != nullptr ? *selected : ItemStack{};
    }
    if (slot >= 1 && slot <= static_cast<int>(inventory.armor.size())) {
        return inventory.armor[static_cast<std::size_t>(slot - 1)];
    }
    return {};
}

void ServerPlayerEntity::tick()
{
    interactionManager.update();
    if (joinInvulnerabilityTicks > 0) {
        --joinInvulnerabilityTicks;
    }
    if (currentScreenHandler != nullptr) {
        currentScreenHandler->sendContentUpdates();
    }
    for (int slot = 0; slot < 5; ++slot) {
        const ItemStack itemStack = getEquipment(slot);
        if (itemStack == equipment_[static_cast<std::size_t>(slot)]) {
            continue;
        }
        equipment_[static_cast<std::size_t>(slot)] = itemStack;
    }
}

void ServerPlayerEntity::playerTick(bool shouldSendChunkUpdates)
{
    PlayerEntity::tick();

    for (std::size_t slot = 0; slot < inventory.size(); ++slot) {
        ItemStack itemStack = inventory.getStack(slot);
        if (itemStack.empty() || itemStack.getItem() == nullptr) {
            continue;
        }
        auto* networkSyncedItem = dynamic_cast<item::NetworkSyncedItem*>(itemStack.getItem());
        if (networkSyncedItem == nullptr || networkHandler == nullptr
            || networkHandler->getBlockDataSendQueueSize() > 2) {
            continue;
        }
        std::unique_ptr<Packet> updatePacket(networkSyncedItem->getUpdatePacket(&itemStack, world, this));
        if (updatePacket == nullptr) {
            continue;
        }
        networkHandler->sendPacket(std::move(updatePacket));
    }

    (void)shouldSendChunkUpdates;

    if (inTeleportationState) {
        if (server != nullptr && server->allowNether) {
            if (currentScreenHandler != &playerScreenHandler) {
                closeHandledScreen();
            }
            if (vehicle != nullptr) {
                setVehicle(vehicle);
            } else {
                changeDimensionCooldown += 0.0125f;
                if (changeDimensionCooldown >= 1.0f) {
                    changeDimensionCooldown = 1.0f;
                    portalCooldown = 10;
                    server->playerManager.changePlayerDimension(this);
                }
            }
            inTeleportationState = false;
        }
    } else {
        if (changeDimensionCooldown > 0.0f) {
            changeDimensionCooldown -= 0.05f;
        }
        if (changeDimensionCooldown < 0.0f) {
            changeDimensionCooldown = 0.0f;
        }
    }

    if (portalCooldown > 0) {
        --portalCooldown;
    }

    if (health != lastHealthScore) {
        lastHealthScore = health;
    }
}

void ServerPlayerEntity::onKilledBy(Entity* adversary)
{
    (void)adversary;
    inventory.dropInventory();
}

bool ServerPlayerEntity::damage(Entity* damageSource, int amount)
{
    if (joinInvulnerabilityTicks > 0) {
        return false;
    }
    if (server != nullptr && !server->pvpEnabled) {
        if (dynamic_cast<PlayerEntity*>(damageSource) != nullptr) {
            return false;
        }
        if (auto* arrow = dynamic_cast<entity::projectile::ArrowEntity*>(damageSource)) {
            if (arrow->owner != nullptr && dynamic_cast<PlayerEntity*>(arrow->owner) != nullptr) {
                return false;
            }
        }
    }
    return PlayerEntity::damage(damageSource, amount);
}

bool ServerPlayerEntity::isPvpEnabled() const
{
    return server != nullptr && server->pvpEnabled;
}

void ServerPlayerEntity::respawn()
{
    if (world == nullptr) {
        return;
    }
    markDead();
    world->remove(this);
    dead = false;
    health = maxHealth;
    deathTime = 0;
    fireTicks = 0;
    if (spawnPos.has_value()) {
        setPositionAndAnglesKeepPrevAngles(
            static_cast<double>(spawnPos->x) + 0.5,
            static_cast<double>(spawnPos->y) + 1.0,
            static_cast<double>(spawnPos->z) + 0.5,
            yaw,
            pitch);
    } else {
        const Vec3i spawn = world->getSpawnPos();
        setPositionAndAnglesKeepPrevAngles(
            static_cast<double>(spawn.x) + 0.5,
            static_cast<double>(spawn.y) + 1.0,
            static_cast<double>(spawn.z) + 0.5,
            yaw,
            pitch);
    }
    velocityX = 0.0;
    velocityY = 0.0;
    velocityZ = 0.0;
    world->spawnEntity(this);
}

SleepAttemptResult ServerPlayerEntity::trySleep(int xIn, int yIn, int zIn)
{
    return PlayerEntity::trySleep(xIn, yIn, zIn);
}

void ServerPlayerEntity::wakeUp(bool resetSleepTimer, bool updateSleepingPlayers, bool setSpawnPosFlag)
{
    PlayerEntity::wakeUp(resetSleepTimer, updateSleepingPlayers, setSpawnPosFlag);
}

void ServerPlayerEntity::increaseStat(int stat, int amount)
{
    if (amount <= 0 || statPacketSender == nullptr || stat::Stats::isLocalOnly(stat)) {
        return;
    }
    int remaining = amount;
    while (remaining > 100) {
        statPacketSender(stat, 100);
        remaining -= 100;
    }
    statPacketSender(stat, remaining);
}

void ServerPlayerEntity::sendMessage(const std::string& message)
{
    (void)message;
}

void ServerPlayerEntity::updateInput(
    float sidewaysSpeedIn,
    float forwardSpeedIn,
    bool jumpingIn,
    bool sneakingIn,
    float pitchIn,
    float yawIn)
{
    sidewaysSpeed = sidewaysSpeedIn;
    forwardSpeed = forwardSpeedIn;
    jumping = jumpingIn;
    setSneaking(sneakingIn);
    pitch = pitchIn;
    yaw = yawIn;
}

void ServerPlayerEntity::handleFall(double heightDifference, bool onGroundIn)
{
    fall(heightDifference, onGroundIn);
}

float ServerPlayerEntity::getEyeHeight() const
{
    return 1.62f;
}

} // namespace net::minecraft::entity::player
