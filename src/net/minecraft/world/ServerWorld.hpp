#pragma once
#include <vector>
#include "net/minecraft/util/IntHashMap.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/explosion/Explosion.hpp"
namespace net::minecraft {
class ChunkSource;
class WorldStorage;
namespace server {
class MinecraftServer;
} // namespace server
namespace world::chunk {
class ChunkCache;
}
class ServerWorld : public World {
 public:
 ServerWorld(server::MinecraftServer* server,
             WorldStorage* storage,
             const std::string& name,
             int dimensionId,
             std::uint64_t seed);
 void updateEntity(Entity* entity, bool requireLoaded);
 void tickVehicle(Entity* vehicle, bool requireLoaded);
 ChunkSource* createChunkCache() override;
 net::minecraft::world::chunk::ChunkCache* chunkCache = nullptr;
 std::vector<block::entity::BlockEntity*> getBlockEntities(
     int minX, int minY, int minZ, int maxX, int maxY, int maxZ);
 bool canInteract(PlayerEntity* player, int x, int y, int z);
 void notifyEntityAdded(Entity* entity) override;
 void notifyEntityRemoved(Entity* entity) override;
 Entity* getEntity(int id);
 bool spawnGlobalEntity(Entity* entity) override;
 void broadcastEntityEvent(Entity* entity, std::uint8_t event) override;
 Explosion createExplosion(Entity* source, double x, double y, double z, float power, bool fire);
 void playNoteBlockActionAt(int x, int y, int z, int soundType, int pitch);
 void forceSave();
 void saveWithLoadingDisplay(bool saveEntities, client::gui::screen::LoadingDisplay* display);
 void updateWeatherCycles() override;
 bool bypassSpawnProtection = false;
 bool savingDisabled = false;
 bool isSavingDisabled() const override { return savingDisabled; }

 private:
 server::MinecraftServer* server_ = nullptr;
 util::IntHashMap<Entity*> entitiesById_;
};
} // namespace net::minecraft
