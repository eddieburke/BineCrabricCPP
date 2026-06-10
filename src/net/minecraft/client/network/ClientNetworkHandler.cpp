#include "net/minecraft/client/network/ClientNetworkHandler.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/entity/DispenserBlockEntity.hpp"
#include "net/minecraft/block/entity/FurnaceBlockEntity.hpp"
#include "net/minecraft/block/entity/SignBlockEntity.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/MultiplayerInteractionManager.hpp"
#include "net/minecraft/client/gui/screen/DisconnectedScreen.hpp"
#include "net/minecraft/client/gui/screen/DownloadingTerrainScreen.hpp"
#include "net/minecraft/client/network/MultiplayerClientPlayerEntity.hpp"
#include "net/minecraft/client/network/OtherPlayerEntity.hpp"
#include "net/minecraft/client/particle/PickupParticle.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/entity/FallingBlockEntity.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/LightningEntity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/TntEntity.hpp"
#include "net/minecraft/entity/decoration/painting/PaintingEntity.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/projectile/ArrowEntity.hpp"
#include "net/minecraft/entity/projectile/FireballEntity.hpp"
#include "net/minecraft/entity/projectile/FishingBobberEntity.hpp"
#include "net/minecraft/entity/projectile/thrown/EggEntity.hpp"
#include "net/minecraft/entity/projectile/thrown/SnowballEntity.hpp"
#include "net/minecraft/entity/vehicle/BoatEntity.hpp"
#include "net/minecraft/entity/vehicle/MinecartEntity.hpp"
#include "net/minecraft/inventory/SimpleInventory.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/item/MapItem.hpp"
#include "net/minecraft/item/map/MapState.hpp"
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/packet/Packets.hpp"
#include "net/minecraft/screen/ScreenHandler.hpp"
#include "net/minecraft/stat/PlayerStats.hpp"
#include "net/minecraft/stat/Stats.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/explosion/Explosion.hpp"

#include <cmath>
#include <memory>
#include <utility>

namespace net::minecraft::client::network {
namespace {

[[nodiscard]] ClientWorld* asClientWorld(World* worldPtr)
{
    return dynamic_cast<ClientWorld*>(worldPtr);
}

[[nodiscard]] float decodePacketYaw(std::int8_t yaw)
{
    return static_cast<float>(yaw * 360) / 256.0f;
}

[[nodiscard]] float decodePacketPitch(std::int8_t pitch)
{
    return static_cast<float>(pitch * 360) / 256.0f;
}

} // namespace

ClientNetworkHandler::ClientNetworkHandler(client::Minecraft* minecraft, std::string address, int port)
    : minecraft(minecraft),
      address_(std::move(address)),
      port_(port)
{
}

ClientNetworkHandler::~ClientNetworkHandler()
{
    if (joinServerThread_.joinable()) {
        joinServerThread_.join();
    }
}

void ClientNetworkHandler::processPendingJoinServer()
{
    if (minecraft == nullptr || disconnected) {
        return;
    }

    JoinServerState state = JoinServerState::None;
    auth::JoinServerResult result;
    {
        std::lock_guard lock(joinServerMutex_);
        state = joinServerState_;
        if (state == JoinServerState::None || state == JoinServerState::Pending) {
            return;
        }
        result = joinServerResult_;
        joinServerState_ = JoinServerState::None;
    }

    if (state == JoinServerState::Succeeded) {
        sendPacket(LoginHelloPacket{minecraft->session.username, 14});
        return;
    }

    if (connection_ == nullptr) {
        return;
    }

    if (!result.error.empty()) {
        connection_->disconnect("disconnect.genericReason", {"Internal client error: " + result.error});
        return;
    }

    connection_->disconnect("disconnect.loginFailedInfo", {result.responseLine});
}

void ClientNetworkHandler::tick()
{
    processPendingJoinServer();
    if (disconnected || connection_ == nullptr) {
        return;
    }
    connection_->tick();
    connection_->interrupt();
}

void ClientNetworkHandler::onHello(std::uint64_t worldSeed, int dimensionId, int protocolVersion)
{
    if (minecraft == nullptr) {
        return;
    }

    minecraft->interactionManager = std::make_unique<MultiplayerInteractionManager>(minecraft, this);
    if (minecraft->stats != nullptr) {
        minecraft->stats->increment(stat::Stats::JOIN_MULTIPLAYER, 1);
    }
    ownedWorld_ = std::make_unique<ClientWorld>(this, worldSeed, dimensionId);
    world = ownedWorld_.get();
    minecraft->setWorld(world);
    if (minecraft->player != nullptr) {
        minecraft->player->dimensionId = dimensionId;
        minecraft->player->id = protocolVersion;
    }
    minecraft->setScreen(std::make_unique<client::gui::screen::DownloadingTerrainScreen>(this));
}

void ClientNetworkHandler::onHello(const LoginHelloPacket& packet)
{
    onHello(packet.worldSeed, packet.dimensionId, packet.protocolVersion);
}

void ClientNetworkHandler::onWorldTimeUpdate(const WorldTimeUpdateS2CPacket& packet)
{
    if (minecraft == nullptr || minecraft->world == nullptr) {
        return;
    }
    ClientWorld* clientWorld = dynamic_cast<ClientWorld*>(minecraft->world);
    if (clientWorld == nullptr) {
        minecraft->world->setTime(static_cast<std::uint64_t>(packet.time));
        return;
    }
    clientWorld->setTime(static_cast<std::uint64_t>(packet.time));
    clientWorld->updateSkyBrightness();
}

void ClientNetworkHandler::disconnect(const std::string& reason)
{
    (void)reason;
    disconnected = true;
    if (minecraft != nullptr) {
        minecraft->setWorld(nullptr);
    }
    ownedWorld_.reset();
    world = nullptr;
    openScreenInventory_.reset();
    openScreenFurnace_.reset();
    openScreenDispenser_.reset();
}

void ClientNetworkHandler::onDisconnected(const std::string& reason, const std::vector<std::string>& objects)
{
    if (disconnected || minecraft == nullptr) {
        return;
    }
    disconnected = true;
    minecraft->setWorld(nullptr);
    ownedWorld_.reset();
    world = nullptr;
    minecraft->setScreen(
        std::make_unique<client::gui::screen::DisconnectedScreen>("disconnect.lost", reason, objects));
}

void ClientNetworkHandler::onHandshake(const HandshakePacket& packet)
{
    if (minecraft == nullptr || disconnected) {
        return;
    }
    if (packet.name == "-") {
        sendPacket(LoginHelloPacket{minecraft->session.username, 14});
        return;
    }

    {
        std::lock_guard lock(joinServerMutex_);
        if (joinServerState_ == JoinServerState::Pending) {
            return;
        }
        joinServerState_ = JoinServerState::Pending;
    }

    const std::string username = minecraft->session.username;
    const std::string sessionId = minecraft->session.sessionId;
    const std::string serverId = packet.name;

    if (joinServerThread_.joinable()) {
        joinServerThread_.join();
    }

    joinServerThread_ = std::thread([this, username, sessionId, serverId]() {
        auth::JoinServerResult result = auth::verifyJoinServer(username, sessionId, serverId);
        std::lock_guard lock(joinServerMutex_);
        joinServerResult_ = std::move(result);
        joinServerState_ = joinServerResult_.ok ? JoinServerState::Succeeded : JoinServerState::Failed;
    });
}

void ClientNetworkHandler::onChatMessage(const ChatMessagePacket& packet)
{
    if (minecraft == nullptr) {
        return;
    }
    minecraft->inGameHud.addChatMessage(packet.chatMessage);
}

void ClientNetworkHandler::onDisconnect(const DisconnectPacket& packet)
{
    disconnected = true;
    if (minecraft != nullptr) {
        minecraft->setWorld(nullptr);
        ownedWorld_.reset();
        world = nullptr;
        minecraft->setScreen(std::make_unique<client::gui::screen::DisconnectedScreen>(
            "disconnect.disconnected", "disconnect.genericReason", std::vector<std::string>{packet.reason}));
    }
}

void ClientNetworkHandler::onItemEntitySpawn(const ItemEntitySpawnS2CPacket& packet)
{
    ClientWorld* clientWorld = asClientWorld(world);
    if (clientWorld == nullptr) {
        return;
    }
    const double x = static_cast<double>(packet.x) / 32.0;
    const double y = static_cast<double>(packet.y) / 32.0;
    const double z = static_cast<double>(packet.z) / 32.0;
    auto* itemEntity = new entity::ItemEntity(
        clientWorld, x, y, z, ItemStack(packet.itemRawId, packet.itemCount, packet.itemDamage));
    itemEntity->velocityX = static_cast<double>(packet.velocityX) / 128.0;
    itemEntity->velocityY = static_cast<double>(packet.velocityY) / 128.0;
    itemEntity->velocityZ = static_cast<double>(packet.velocityZ) / 128.0;
    itemEntity->trackedPosX = packet.x;
    itemEntity->trackedPosY = packet.y;
    itemEntity->trackedPosZ = packet.z;
    clientWorld->forceEntity(packet.id, itemEntity);
}

void ClientNetworkHandler::onEntitySpawn(const EntitySpawnS2CPacket& packet)
{
    ClientWorld* clientWorld = asClientWorld(world);
    if (clientWorld == nullptr) {
        return;
    }

    const double x = static_cast<double>(packet.x) / 32.0;
    const double y = static_cast<double>(packet.y) / 32.0;
    const double z = static_cast<double>(packet.z) / 32.0;
    entity::Entity* entity = nullptr;

    switch (packet.entityType) {
    case 10:
        entity = new entity::vehicle::MinecartEntity(clientWorld, x, y, z, 0);
        break;
    case 11:
        entity = new entity::vehicle::MinecartEntity(clientWorld, x, y, z, 1);
        break;
    case 12:
        entity = new entity::vehicle::MinecartEntity(clientWorld, x, y, z, 2);
        break;
    case 90: {
        auto* bobber = new entity::projectile::FishingBobberEntity(clientWorld);
        bobber->setPosition(x, y, z);
        entity = bobber;
        break;
    }
    case 60:
        entity = new entity::projectile::ArrowEntity(clientWorld, x, y, z);
        break;
    case 61:
        entity = new entity::projectile::thrown::SnowballEntity(clientWorld, x, y, z);
        break;
    case 63:
        entity = new entity::projectile::FireballEntity(
            clientWorld,
            x,
            y,
            z,
            static_cast<double>(packet.velocityX) / 8000.0,
            static_cast<double>(packet.velocityY) / 8000.0,
            static_cast<double>(packet.velocityZ) / 8000.0);
        break;
    case 62:
        entity = new entity::projectile::thrown::EggEntity(clientWorld, x, y, z);
        break;
    case 1:
        entity = new entity::vehicle::BoatEntity(clientWorld, x, y, z);
        break;
    case 50:
        entity = new entity::TntEntity(clientWorld, x, y, z);
        break;
    case 70:
        entity = new entity::FallingBlockEntity(
            clientWorld, x, y, z, Block::SAND != nullptr ? Block::SAND->id : 12);
        break;
    case 71:
        entity = new entity::FallingBlockEntity(
            clientWorld, x, y, z, Block::GRAVEL != nullptr ? Block::GRAVEL->id : 13);
        break;
    default:
        break;
    }

    if (entity == nullptr) {
        return;
    }

    entity->trackedPosX = packet.x;
    entity->trackedPosY = packet.y;
    entity->trackedPosZ = packet.z;
    entity->yaw = 0.0f;
    entity->pitch = 0.0f;
    entity->id = packet.id;
    clientWorld->forceEntity(packet.id, entity);

    if (packet.entityData > 0 && packet.entityType != 63) {
        if (packet.entityType == 60) {
            if (Entity* ownerEntity = getEntity(packet.entityData)) {
                if (auto* arrow = dynamic_cast<entity::projectile::ArrowEntity*>(entity)) {
                    if (auto* owner = dynamic_cast<entity::LivingEntity*>(ownerEntity)) {
                        arrow->owner = owner;
                    }
                }
            }
        }
        entity->setVelocityClient(
            static_cast<double>(packet.velocityX) / 8000.0,
            static_cast<double>(packet.velocityY) / 8000.0,
            static_cast<double>(packet.velocityZ) / 8000.0);
    }
}

void ClientNetworkHandler::onLightningEntitySpawn(const GlobalEntitySpawnS2CPacket& packet)
{
    if (world == nullptr || packet.type != 1) {
        return;
    }
    const double x = static_cast<double>(packet.x) / 32.0;
    const double y = static_cast<double>(packet.y) / 32.0;
    const double z = static_cast<double>(packet.z) / 32.0;
    auto* lightningEntity = new entity::LightningEntity(world, x, y, z);
    lightningEntity->trackedPosX = packet.x;
    lightningEntity->trackedPosY = packet.y;
    lightningEntity->trackedPosZ = packet.z;
    lightningEntity->yaw = 0.0f;
    lightningEntity->pitch = 0.0f;
    lightningEntity->id = packet.id;
    world->spawnGlobalEntity(lightningEntity);
}

void ClientNetworkHandler::onPaintingEntitySpawn(const PaintingEntitySpawnS2CPacket& packet)
{
    ClientWorld* clientWorld = asClientWorld(world);
    if (clientWorld == nullptr) {
        return;
    }
    auto* paintingEntity = new entity::decoration::painting::PaintingEntity(
        clientWorld, packet.x, packet.y, packet.z, packet.facing, packet.variant);
    clientWorld->forceEntity(packet.id, paintingEntity);
}

void ClientNetworkHandler::onEntityVelocityUpdate(const EntityVelocityUpdateS2CPacket& packet)
{
    Entity* entity = getEntity(packet.id);
    if (entity == nullptr) {
        return;
    }
    entity->setVelocityClient(
        static_cast<double>(packet.velocityX) / 8000.0,
        static_cast<double>(packet.velocityY) / 8000.0,
        static_cast<double>(packet.velocityZ) / 8000.0);
}

void ClientNetworkHandler::onEntityTrackerUpdate(const EntityTrackerUpdateS2CPacket& packet)
{
    Entity* entity = getEntity(packet.id);
    if (entity == nullptr || packet.trackedValues.empty()) {
        return;
    }
    entity->getDataTracker().writeUpdatedEntries(packet.trackedValues);
}

void ClientNetworkHandler::onPlayerSpawn(const PlayerSpawnS2CPacket& packet)
{
    ClientWorld* clientWorld = asClientWorld(world);
    if (clientWorld == nullptr || minecraft == nullptr) {
        return;
    }

    const double x = static_cast<double>(packet.x) / 32.0;
    const double y = static_cast<double>(packet.y) / 32.0;
    const double z = static_cast<double>(packet.z) / 32.0;
    const float yaw = decodePacketYaw(packet.yaw);
    const float pitch = decodePacketPitch(packet.pitch);

    auto* otherPlayer = new OtherPlayerEntity(minecraft->world, packet.name);
    otherPlayer->trackedPosX = packet.x;
    otherPlayer->prevX = otherPlayer->lastTickX = static_cast<double>(otherPlayer->trackedPosX);
    otherPlayer->trackedPosY = packet.y;
    otherPlayer->prevY = otherPlayer->lastTickY = static_cast<double>(otherPlayer->trackedPosY);
    otherPlayer->trackedPosZ = packet.z;
    otherPlayer->prevZ = otherPlayer->lastTickZ = static_cast<double>(otherPlayer->trackedPosZ);
    if (packet.itemRawId == 0) {
        otherPlayer->inventory.main[static_cast<std::size_t>(otherPlayer->inventory.selectedSlot)] = {};
    } else {
        otherPlayer->inventory.main[static_cast<std::size_t>(otherPlayer->inventory.selectedSlot)] =
            ItemStack(packet.itemRawId, 1, 0);
    }
    otherPlayer->setPositionAndAngles(x, y, z, yaw, pitch);
    clientWorld->forceEntity(packet.id, otherPlayer);
}

void ClientNetworkHandler::onEntityPosition(const EntityPositionS2CPacket& packet)
{
    Entity* entity = getEntity(packet.id);
    if (entity == nullptr) {
        return;
    }
    entity->trackedPosX = packet.x;
    entity->trackedPosY = packet.y;
    entity->trackedPosZ = packet.z;
    const double x = static_cast<double>(entity->trackedPosX) / 32.0;
    const double y = static_cast<double>(entity->trackedPosY) / 32.0 + 0.015625;
    const double z = static_cast<double>(entity->trackedPosZ) / 32.0;
    const float yaw = decodePacketYaw(packet.yaw);
    const float pitch = decodePacketPitch(packet.pitch);
    setEntityPositionAndAnglesAvoidEntities(entity, x, y, z, yaw, pitch, 3);
}

void ClientNetworkHandler::onEntity(const EntityS2CPacket& packet)
{
    Entity* entity = getEntity(packet.id);
    if (entity == nullptr) {
        return;
    }
    entity->trackedPosX += packet.deltaX;
    entity->trackedPosY += packet.deltaY;
    entity->trackedPosZ += packet.deltaZ;
    const double x = static_cast<double>(entity->trackedPosX) / 32.0;
    const double y = static_cast<double>(entity->trackedPosY) / 32.0;
    const double z = static_cast<double>(entity->trackedPosZ) / 32.0;
    const float yaw = packet.rotate ? decodePacketYaw(packet.yaw) : entity->yaw;
    const float pitch = packet.rotate ? decodePacketPitch(packet.pitch) : entity->pitch;
    setEntityPositionAndAnglesAvoidEntities(entity, x, y, z, yaw, pitch, 3);
}

void ClientNetworkHandler::onEntityDestroy(const EntityDestroyS2CPacket& packet)
{
    ClientWorld* clientWorld = asClientWorld(world);
    if (clientWorld == nullptr) {
        return;
    }
    clientWorld->removeEntity(packet.id);
}

void ClientNetworkHandler::onPlayerMove(const PlayerMovePacket& packet)
{
    if (minecraft == nullptr || minecraft->player == nullptr) {
        return;
    }

    entity::player::ClientPlayerEntity* player = minecraft->player;
    double x = player->x;
    double y = player->y;
    double z = player->z;
    float yaw = player->yaw;
    float pitch = player->pitch;
    if (packet.changePosition) {
        x = packet.x;
        y = packet.y;
        z = packet.z;
    }
    if (packet.changeLook) {
        yaw = packet.yaw;
        pitch = packet.pitch;
    }

    player->cameraOffset = 0.0f;
    player->velocityZ = 0.0;
    player->velocityY = 0.0;
    player->velocityX = 0.0;
    player->setPositionAndAngles(x, y, z, yaw, pitch);

    PlayerMovePacket response = packet;
    response.x = player->x;
    response.y = player->boundingBox.minY;
    response.z = player->z;
    response.eyeHeight = player->y;
    sendPacket(response);

    if (!started) {
        player->prevX = player->x;
        player->prevY = player->y;
        player->prevZ = player->z;
        started = true;
        minecraft->setScreen(nullptr);
    }
}

void ClientNetworkHandler::onChunkStatusUpdate(const ChunkStatusUpdateS2CPacket& packet)
{
    ClientWorld* clientWorld = asClientWorld(world);
    if (clientWorld == nullptr) {
        return;
    }
    clientWorld->updateChunk(packet.x, packet.z, packet.load);
}

void ClientNetworkHandler::onChunkDeltaUpdate(const ChunkDeltaUpdateS2CPacket& packet)
{
    ClientWorld* clientWorld = asClientWorld(world);
    if (clientWorld == nullptr || world == nullptr) {
        return;
    }

    Chunk& chunk = world->getChunk(packet.x, packet.z);
    const int baseX = packet.x * 16;
    const int baseZ = packet.z * 16;
    for (int i = 0; i < packet.entryCount; ++i) {
        const std::int16_t position = packet.positions[static_cast<std::size_t>(i)];
        const int blockId = static_cast<int>(packet.blockRawIds[static_cast<std::size_t>(i)]) & 0xFF;
        const int meta = static_cast<int>(packet.blockMetadata[static_cast<std::size_t>(i)]);
        const int localX = (position >> 12) & 0xF;
        const int localZ = (position >> 8) & 0xF;
        const int localY = position & 0xFF;
        chunk.setBlock(localX, localY, localZ, blockId, meta);
        const int worldX = localX + baseX;
        const int worldZ = localZ + baseZ;
        clientWorld->clearBlockResets(worldX, localY, worldZ, worldX, localY, worldZ);
        world->blockUpdate(worldX, localY, worldZ, blockId);
        clientWorld->setBlocksDirty(worldX, localY, worldZ, worldX, localY, worldZ);
    }
}

void ClientNetworkHandler::handleChunkData(const ChunkDataS2CPacket& packet)
{
    ClientWorld* clientWorld = asClientWorld(world);
    if (clientWorld == nullptr || world == nullptr) {
        return;
    }
    clientWorld->clearBlockResets(
        packet.x, packet.y, packet.z, packet.x + packet.sizeX - 1, packet.y + packet.sizeY - 1, packet.z + packet.sizeZ - 1);
    world->handleChunkDataUpdate(
        packet.x, packet.y, packet.z, packet.sizeX, packet.sizeY, packet.sizeZ, packet.chunkData);
}

void ClientNetworkHandler::onBlockUpdate(const BlockUpdateS2CPacket& packet)
{
    ClientWorld* clientWorld = asClientWorld(world);
    if (clientWorld == nullptr) {
        return;
    }
    clientWorld->setBlockWithMetaFromPacket(packet.x, packet.y, packet.z, packet.blockRawId, packet.blockMetadata);
}

void ClientNetworkHandler::onItemPickupAnimation(const ItemPickupAnimationS2CPacket& packet)
{
    if (minecraft == nullptr || world == nullptr) {
        return;
    }

    Entity* entity = getEntity(packet.entityId);
    entity::LivingEntity* collector = dynamic_cast<entity::LivingEntity*>(getEntity(packet.collectorEntityId));
    if (collector == nullptr) {
        collector = minecraft->player;
    }
    if (entity == nullptr || collector == nullptr) {
        return;
    }

    world->playSound(
        entity,
        "random.pop",
        0.2f,
        ((random.nextFloat() - random.nextFloat()) * 0.7f + 1.0f) * 2.0f);
    minecraft->particleManager.addParticle(
        new client::particle::PickupParticle(world, entity, collector, -0.5f));
    if (ClientWorld* clientWorld = asClientWorld(world)) {
        clientWorld->removeEntity(packet.entityId);
    }
}

void ClientNetworkHandler::onEntityAnimation(const EntityAnimationPacket& packet)
{
    Entity* entity = getEntity(packet.id);
    if (entity == nullptr) {
        return;
    }
    auto* player = dynamic_cast<entity::player::PlayerEntity*>(entity);
    if (packet.animationId == 1) {
        if (player != nullptr) {
            player->swingHand();
        }
    } else if (packet.animationId == 2) {
        entity->animateHurt();
    } else if (packet.animationId == 3) {
        if (player != nullptr) {
            player->wakeUp(false, false, false);
        }
    } else if (packet.animationId == 4) {
        if (auto* otherPlayer = dynamic_cast<OtherPlayerEntity*>(entity)) {
            otherPlayer->spawn();
        }
    }
}

void ClientNetworkHandler::onPlayerSleepUpdate(const PlayerSleepUpdateS2CPacket& packet)
{
    Entity* entity = getEntity(packet.id);
    if (entity == nullptr || packet.status != 0) {
        return;
    }
    if (auto* player = dynamic_cast<entity::player::PlayerEntity*>(entity)) {
        player->trySleep(packet.x, packet.y, packet.z);
    }
}

void ClientNetworkHandler::onLivingEntitySpawn(const LivingEntitySpawnS2CPacket& packet)
{
    ClientWorld* clientWorld = asClientWorld(world);
    if (clientWorld == nullptr || minecraft == nullptr || minecraft->world == nullptr) {
        return;
    }

    const double x = static_cast<double>(packet.x) / 32.0;
    const double y = static_cast<double>(packet.y) / 32.0;
    const double z = static_cast<double>(packet.z) / 32.0;
    const float yaw = decodePacketYaw(packet.yaw);
    const float pitch = decodePacketPitch(packet.pitch);

    std::unique_ptr<entity::Entity> created =
        entity::EntityRegistry::create(static_cast<int>(packet.entityType), minecraft->world);
    auto* livingEntity = dynamic_cast<entity::LivingEntity*>(created.get());
    if (livingEntity == nullptr) {
        return;
    }

    livingEntity->trackedPosX = packet.x;
    livingEntity->trackedPosY = packet.y;
    livingEntity->trackedPosZ = packet.z;
    livingEntity->id = packet.id;
    livingEntity->setPositionAndAngles(x, y, z, yaw, pitch);
    livingEntity->interpolateOnly = true;
    clientWorld->forceEntity(packet.id, created.release());
    if (!packet.trackedValues.empty()) {
        livingEntity->getDataTracker().writeUpdatedEntries(packet.trackedValues);
    }
}

void ClientNetworkHandler::onPlayerSpawnPosition(const PlayerSpawnPositionS2CPacket& packet)
{
    if (minecraft == nullptr || minecraft->player == nullptr || world == nullptr) {
        return;
    }
    minecraft->player->setSpawnPos(Vec3i{packet.x, packet.y, packet.z});
    world->getProperties().setSpawn(packet.x, packet.y, packet.z);
}

void ClientNetworkHandler::onEntityVehicleSet(const EntityVehicleSetS2CPacket& packet)
{
    Entity* entity = getEntity(packet.id);
    Entity* vehicle = getEntity(packet.vehicleId);
    if (minecraft != nullptr && packet.id == minecraft->player->id) {
        entity = minecraft->player;
    }
    if (entity == nullptr) {
        return;
    }
    // Reject cycle: vehicle must not already be a passenger of entity.
    if (vehicle != nullptr && vehicle->vehicle == entity) {
        return;
    }
    entity->setVehicle(vehicle);
}

void ClientNetworkHandler::onEntityStatus(const EntityStatusS2CPacket& packet)
{
    Entity* entity = getEntity(packet.id);
    if (entity != nullptr) {
        entity->processServerEntityStatus(packet.status);
    }
}

void ClientNetworkHandler::onHealthUpdate(const HealthUpdateS2CPacket& packet)
{
    if (minecraft == nullptr || minecraft->player == nullptr) {
        return;
    }
    if (auto* multiplayerPlayer = dynamic_cast<MultiplayerClientPlayerEntity*>(minecraft->player)) {
        multiplayerPlayer->damageTo(packet.health);
    } else {
        minecraft->player->health = packet.health;
    }
}

void ClientNetworkHandler::onPlayerRespawn(const PlayerRespawnPacket& packet)
{
    if (minecraft == nullptr || minecraft->player == nullptr || world == nullptr) {
        return;
    }

    if (packet.dimensionRawId != minecraft->player->dimensionId) {
        started = false;
        const std::uint64_t seed = world->getSeed();
        ownedWorld_ = std::make_unique<ClientWorld>(this, seed, packet.dimensionRawId);
        world = ownedWorld_.get();
        minecraft->setWorld(world);
        minecraft->player->dimensionId = packet.dimensionRawId;
        minecraft->setScreen(std::make_unique<client::gui::screen::DownloadingTerrainScreen>(this));
    }
    minecraft->respawnPlayer(true, packet.dimensionRawId);
}

void ClientNetworkHandler::onExplosion(const ExplosionS2CPacket& packet)
{
    if (minecraft == nullptr || minecraft->world == nullptr) {
        return;
    }
    Explosion explosion(minecraft->world, nullptr, packet.x, packet.y, packet.z, packet.radius);
    explosion.damagedBlocks.insert(packet.affectedBlocks.begin(), packet.affectedBlocks.end());
    explosion.playExplosionSound(true);
}

void ClientNetworkHandler::onOpenScreen(const OpenScreenS2CPacket& packet)
{
    if (minecraft == nullptr || minecraft->player == nullptr) {
        return;
    }

    auto* player = minecraft->player;
    if (packet.screenHandlerId == 0) {
        openScreenInventory_ = std::make_unique<SimpleInventory>(packet.name, static_cast<std::size_t>(packet.inventorySize));
        player->openChestScreen(openScreenInventory_.get());
        player->currentScreenHandler->syncId = packet.syncId;
    } else if (packet.screenHandlerId == 2) {
        openScreenFurnace_ = std::make_unique<block::entity::FurnaceBlockEntity>();
        player->openFurnaceScreen(openScreenFurnace_.get());
        player->currentScreenHandler->syncId = packet.syncId;
    } else if (packet.screenHandlerId == 3) {
        openScreenDispenser_ = std::make_unique<block::entity::DispenserBlockEntity>();
        player->openDispenserScreen(openScreenDispenser_.get());
        player->currentScreenHandler->syncId = packet.syncId;
    } else if (packet.screenHandlerId == 1) {
        player->openCraftingScreen(
            MathHelper::floor(player->x), MathHelper::floor(player->y), MathHelper::floor(player->z));
        player->currentScreenHandler->syncId = packet.syncId;
    }
}

void ClientNetworkHandler::onScreenHandlerSlotUpdate(const ScreenHandlerSlotUpdateS2CPacket& packet)
{
    if (minecraft == nullptr || minecraft->player == nullptr) {
        return;
    }

    auto* player = minecraft->player;
    if (packet.syncId == -1) {
        player->inventory.setCursorStack(packet.stack);
        return;
    }
    if (packet.syncId == 0 && packet.slot >= 36 && packet.slot < 45) {
        if (screen::slot::Slot* slot = player->playerScreenHandler.getSlot(packet.slot)) {
            ItemStack existing = slot->getStack();
            ItemStack incoming = packet.stack;
            if (!incoming.empty() && (existing.empty() || existing.count < incoming.count)) {
                incoming.bobbingAnimationTime = 5;
            }
            player->playerScreenHandler.setStackInSlot(packet.slot, incoming);
        }
        return;
    }
    if (player->currentScreenHandler != nullptr && packet.syncId == player->currentScreenHandler->syncId) {
        player->currentScreenHandler->setStackInSlot(packet.slot, packet.stack);
    }
}

void ClientNetworkHandler::onScreenHandlerAcknowledgement(const ScreenHandlerAcknowledgementPacket& packet)
{
    if (minecraft == nullptr || minecraft->player == nullptr) {
        return;
    }

    screen::ScreenHandler* screenHandler = nullptr;
    if (packet.syncId == 0) {
        screenHandler = &minecraft->player->playerScreenHandler;
    } else if (minecraft->player->currentScreenHandler != nullptr
        && packet.syncId == minecraft->player->currentScreenHandler->syncId) {
        screenHandler = minecraft->player->currentScreenHandler;
    }
    if (screenHandler == nullptr) {
        return;
    }
    if (packet.accepted) {
        screenHandler->onAcknowledgementAccepted(static_cast<std::uint16_t>(packet.actionType));
    } else {
        screenHandler->onAcknowledgementDenied(static_cast<std::uint16_t>(packet.actionType));
        sendPacket(ScreenHandlerAcknowledgementPacket{packet.syncId, packet.actionType, true});
    }
}

void ClientNetworkHandler::onInventory(const InventoryS2CPacket& packet)
{
    if (minecraft == nullptr || minecraft->player == nullptr) {
        return;
    }

    auto* player = minecraft->player;
    if (packet.syncId == 0) {
        player->playerScreenHandler.updateSlotStacks(packet.contents);
    } else if (player->currentScreenHandler != nullptr && packet.syncId == player->currentScreenHandler->syncId) {
        player->currentScreenHandler->updateSlotStacks(packet.contents);
    }
}

void ClientNetworkHandler::handleUpdateSign(const UpdateSignPacket& packet)
{
    if (minecraft == nullptr || minecraft->world == nullptr) {
        return;
    }
    if (!minecraft->world->isPosLoaded(packet.x, packet.y, packet.z)) {
        return;
    }
    auto* blockEntity = minecraft->world->getBlockEntity(packet.x, packet.y, packet.z);
    auto* sign = dynamic_cast<block::entity::SignBlockEntity*>(blockEntity);
    if (sign == nullptr) {
        return;
    }
    for (int i = 0; i < 4; ++i) {
        sign->texts[static_cast<std::size_t>(i)] = packet.text[static_cast<std::size_t>(i)];
    }
    sign->markDirty();
}

void ClientNetworkHandler::onScreenHandlerPropertyUpdate(const ScreenHandlerPropertyUpdateS2CPacket& packet)
{
    if (minecraft == nullptr || minecraft->player == nullptr || minecraft->player->currentScreenHandler == nullptr) {
        return;
    }
    if (minecraft->player->currentScreenHandler->syncId == packet.syncId) {
        minecraft->player->currentScreenHandler->setProperty(packet.propertyId, packet.value);
    }
}

void ClientNetworkHandler::onEntityEquipmentUpdate(const EntityEquipmentUpdateS2CPacket& packet)
{
    Entity* entity = getEntity(packet.id);
    if (entity != nullptr) {
        entity->setEquipmentStack(packet.slot, packet.itemRawId, packet.itemDamage);
    }
}

void ClientNetworkHandler::onCloseScreen(const CloseScreenS2CPacket& /*packet*/)
{
    if (minecraft == nullptr || minecraft->player == nullptr) {
        return;
    }
    if (auto* multiplayerPlayer = dynamic_cast<MultiplayerClientPlayerEntity*>(minecraft->player)) {
        multiplayerPlayer->closeHandledScreen();
    } else {
        minecraft->player->closeHandledScreen();
    }
    openScreenInventory_.reset();
    openScreenFurnace_.reset();
    openScreenDispenser_.reset();
}

void ClientNetworkHandler::onPlayNoteSound(const PlayNoteSoundS2CPacket& packet)
{
    if (minecraft == nullptr || minecraft->world == nullptr) {
        return;
    }
    minecraft->world->playNoteBlockActionAt(packet.x, packet.y, packet.z, packet.instrument, packet.pitch);
}

void ClientNetworkHandler::onGameStateChange(const GameStateChangeS2CPacket& packet)
{
    if (minecraft == nullptr || minecraft->player == nullptr || world == nullptr) {
        return;
    }

    const int reason = packet.reason;
    static const char* const kReasonMessages[] = {nullptr, nullptr, nullptr};
    if (reason >= 0 && reason < 3 && kReasonMessages[reason] != nullptr) {
        minecraft->player->sendMessage(kReasonMessages[reason]);
    }
    if (reason == 1) {
        world->getProperties().setRaining(true);
        world->setRainGradient(1.0f);
    } else if (reason == 2) {
        world->getProperties().setRaining(false);
        world->setRainGradient(0.0f);
    }
}

void ClientNetworkHandler::onMapUpdate(const MapUpdateS2CPacket& packet)
{
    if (minecraft == nullptr || minecraft->world == nullptr || Item::byRawId(102) == nullptr) {
        return;
    }
    if (packet.itemRawId == Item::byRawId(102)->id) {
        if (map::MapState* mapState =
                item::MapItem::getMapState(static_cast<std::int16_t>(packet.id), minecraft->world)) {
            mapState->readUpdateData(packet.updateData);
        }
    }
}

void ClientNetworkHandler::onWorldEvent(const WorldEventS2CPacket& packet)
{
    if (minecraft == nullptr || minecraft->world == nullptr) {
        return;
    }
    minecraft->world->worldEvent(packet.eventId, packet.x, packet.y, packet.z, packet.data);
}

void ClientNetworkHandler::onIncreaseStat(const IncreaseStatS2CPacket& packet)
{
    if (minecraft == nullptr || minecraft->player == nullptr) {
        return;
    }
    if (auto* multiplayerPlayer = dynamic_cast<MultiplayerClientPlayerEntity*>(minecraft->player)) {
        multiplayerPlayer->handleIncreaseStat(packet.statId, packet.amount);
    } else {
        minecraft->player->increaseStat(packet.statId, packet.amount);
    }
}

Entity* ClientNetworkHandler::getEntity(int id)
{
    if (minecraft == nullptr || minecraft->player == nullptr || world == nullptr) {
        return nullptr;
    }
    if (id == minecraft->player->id) {
        return minecraft->player;
    }
    if (ClientWorld* clientWorld = asClientWorld(world)) {
        return clientWorld->getEntity(id);
    }
    return nullptr;
}

void ClientNetworkHandler::setEntityPositionAndAnglesAvoidEntities(
    entity::Entity* entity,
    double x,
    double y,
    double z,
    float yaw,
    float pitch,
    int steps)
{
    if (entity == nullptr) {
        return;
    }
    entity->setPositionAndAnglesAvoidEntities(x, y, z, yaw, pitch, steps);
}

} // namespace net::minecraft::client::network
