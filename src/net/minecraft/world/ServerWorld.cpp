#include "net/minecraft/world/ServerWorld.hpp"

#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/WaterCreatureEntity.hpp"
#include "net/minecraft/entity/passive/AnimalEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/chunk/ServerChunkCache.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/chunk/storage/ChunkStorage.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#include "net/minecraft/world/explosion/Explosion.hpp"
#include "net/minecraft/world/storage/WorldStorage.hpp"

#include <algorithm>
#include <cmath>

namespace net::minecraft {

ServerWorld::ServerWorld(
    server::MinecraftServer* server,
    WorldStorage* storage,
    const std::string& name,
    int dimensionId,
    std::uint64_t seed)
    : World(storage, name, static_cast<std::int64_t>(seed))
{
    server_ = server;
    isRemote_ = false;
    if (dimension != nullptr && dimension->id != dimensionId) {
        dimension = Dimension::fromId(dimensionId);
        if (dimension != nullptr) {
            dimension->setWorld(this);
        }
    }
    createChunkCache();
}

void ServerWorld::updateEntity(Entity* entity, bool requireLoaded)
{
    if (entity == nullptr) {
        return;
    }
    if (server_ != nullptr && !server_->spawnAnimals) {
        if (dynamic_cast<entity::passive::AnimalEntity*>(entity) != nullptr
            || dynamic_cast<entity::WaterCreatureEntity*>(entity) != nullptr) {
            entity->markDead();
        }
    }
    if (entity->passenger == nullptr || dynamic_cast<PlayerEntity*>(entity->passenger) == nullptr) {
        World::updateEntity(entity, requireLoaded);
    }
}

void ServerWorld::tickVehicle(Entity* vehicle, bool requireLoaded)
{
    World::updateEntity(vehicle, requireLoaded);
}

ChunkSource* ServerWorld::createChunkCache()
{
    WorldStorage* storage = getDimensionData();
    if (storage == nullptr || dimension == nullptr) {
        return getChunkSource();
    }
    std::unique_ptr<ChunkStorage> chunkStorage = storage->getChunkStorage(dimension.get());
    chunkGeneratorSource_ = dimension->createChunkGenerator();
    setChunkCache(std::make_unique<ServerChunkCache>(this, std::move(chunkStorage), chunkGeneratorSource_.get()));
    chunkCache = dynamic_cast<ServerChunkCache*>(getChunkSource());
    return chunkCache;
}

std::vector<block::entity::BlockEntity*> ServerWorld::getBlockEntities(int minX, int minY, int minZ, int maxX, int maxY, int maxZ)
{
    std::vector<block::entity::BlockEntity*> result;
    for (block::entity::BlockEntity* blockEntity : blockEntities) {
        if (blockEntity == nullptr) {
            continue;
        }
        if (blockEntity->x < minX || blockEntity->y < minY || blockEntity->z < minZ || blockEntity->x >= maxX
            || blockEntity->y >= maxY || blockEntity->z >= maxZ) {
            continue;
        }
        result.push_back(blockEntity);
    }
    return result;
}

bool ServerWorld::canInteract(PlayerEntity* player, int x, int y, int z)
{
    (void)y;
    if (bypassSpawnProtection) {
        return true;
    }
    if (player == nullptr) {
        return false;
    }
    const int spawnX = getProperties().getSpawnX();
    const int spawnZ = getProperties().getSpawnZ();
    int dx = static_cast<int>(MathHelper::abs(x - spawnX));
    int dz = static_cast<int>(MathHelper::abs(z - spawnZ));
    const int distance = std::max(dx, dz);
    return distance > 16 || (server_ != nullptr && server_->isOperator(player->name));
}

void ServerWorld::notifyEntityAdded(Entity* entity)
{
    World::notifyEntityAdded(entity);
    if (entity != nullptr) {
        entitiesById_.put(entity->id, entity);
        if (server_ != nullptr && dimension != nullptr) {
            server_->getEntityTracker(dimension->id).onEntityAdded(entity);
        }
    }
}

void ServerWorld::notifyEntityRemoved(Entity* entity)
{
    World::notifyEntityRemoved(entity);
    if (entity != nullptr) {
        entitiesById_.remove(entity->id);
        if (server_ != nullptr && dimension != nullptr) {
            server_->getEntityTracker(dimension->id).onEntityRemoved(entity);
        }
    }
}

Entity* ServerWorld::getEntity(int id)
{
    return entitiesById_.get(id);
}

bool ServerWorld::spawnGlobalEntity(Entity* entity)
{
    if (!World::spawnGlobalEntity(entity)) {
        return false;
    }
    if (server_ != nullptr && dimension != nullptr && entity != nullptr) {
        server_->playerManager.sendToAround(
            entity->x,
            entity->y,
            entity->z,
            512.0,
            dimension->id);
    }
    return true;
}

void ServerWorld::broadcastEntityEvent(Entity* entity, std::uint8_t event)
{
    (void)event;
    if (server_ == nullptr || dimension == nullptr || entity == nullptr) {
        return;
    }
    server_->getEntityTracker(dimension->id).sendToAround(entity);
}

Explosion ServerWorld::createExplosion(Entity* source, double x, double y, double z, float power, bool fire)
{
    Explosion explosion(this, source, x, y, z, power);
    explosion.fire = fire;
    explosion.explode();
    explosion.playExplosionSound(false);
    if (server_ != nullptr && dimension != nullptr) {
        server_->playerManager.sendToAround(x, y, z, 64.0, dimension->id);
    }
    return explosion;
}

void ServerWorld::playNoteBlockActionAt(int x, int y, int z, int soundType, int pitch)
{
    World::playNoteBlockActionAt(x, y, z, soundType, pitch);
    if (server_ != nullptr && dimension != nullptr) {
        server_->playerManager.sendToAround(
            static_cast<double>(x),
            static_cast<double>(y),
            static_cast<double>(z),
            64.0,
            dimension->id);
    }
}

void ServerWorld::forceSave()
{
    if (getDimensionData() != nullptr) {
        getDimensionData()->forceSave();
    }
}

void ServerWorld::updateWeatherCycles()
{
    const bool wasRaining = isRaining();
    World::updateWeatherCycles();
    if (server_ == nullptr || wasRaining == isRaining()) {
        return;
    }
    if (wasRaining) {
        server_->playerManager.sendToAll();
    } else {
        server_->playerManager.sendToAll();
    }
}

} // namespace net::minecraft
