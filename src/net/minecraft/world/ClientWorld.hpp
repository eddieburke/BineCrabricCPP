#pragma once
#include <list>
#include <unordered_map>
#include <unordered_set>
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::client::multiplayer {
class ClientNetworkHandler;
}
namespace net::minecraft {
class ChunkSource;
class ClientWorld : public World {
 public:
 ClientWorld(client::multiplayer::ClientNetworkHandler* networkHandler, std::uint64_t seed, int dimensionId);
 ChunkSource* createChunkCache() override;
 void tick() override;
 void updateWeatherCycles() override;
 void clearBlockResets(int minX, int minY, int minZ, int maxX, int maxY, int maxZ);
 void updateSpawnPosition() {
  setSpawnPos(Vec3i{8, 64, 8});
 }
 void scheduleBlockUpdate(int x, int y, int z, int id, int tickRate) override {
  (void)x;
  (void)y;
  (void)z;
  (void)id;
  (void)tickRate;
 }
 bool processScheduledTicks(bool flush) override {
  (void)flush;
  return false;
 }
 void updateChunk(int chunkX, int chunkZ, bool load);
 bool spawnEntity(Entity* entity) override;
 void remove(Entity* entity) override;
 void notifyEntityAdded(Entity* entity) override;
 void notifyEntityRemoved(Entity* entity) override;
 void forceEntity(int id, Entity* entity);
 Entity* getEntity(int id);
 Entity* removeEntity(int id);
 bool setBlockMetaWithoutNotifyingNeighbors(int x, int y, int z, int meta) override;
 bool setBlockWithoutNotifyingNeighbors(int x, int y, int z, int blockId, int meta) override;
 bool setBlockWithMetaFromPacket(int x, int y, int z, int blockId, int meta);
 void disconnect();
 [[nodiscard]] client::multiplayer::ClientNetworkHandler* networkHandler() const {
  return networkHandler_;
 }
 struct BlockReset {
  int x = 0;
  int y = 0;
  int z = 0;
  int delay = 80;
  int block = 0;
  int meta = 0;
  BlockReset(int xIn, int yIn, int zIn, int blockIn, int metaIn)
      : x(xIn), y(yIn), z(zIn), block(blockIn), meta(metaIn) {
  }
 };

 protected:
 void manageChunkUpdatesAndEvents() override {
 }

 private:
 void addBlockReset(int x, int y, int z, int block, int meta);
 std::list<BlockReset> blockResets_;
 client::multiplayer::ClientNetworkHandler* networkHandler_ = nullptr;
 std::unordered_map<int, Entity*> entitiesByNetworkId_;
 std::unordered_set<Entity*> forcedEntities_;
 std::unordered_set<Entity*> pendingEntities_;
};
} // namespace net::minecraft
