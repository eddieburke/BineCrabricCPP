#pragma once
#include <string>
#include "net/minecraft/entity/EntityTypes.hpp"
namespace net::minecraft::block::entity {
class BlockEntity;
}
namespace net::minecraft {
class GameEventListener {
 public:
 virtual ~GameEventListener() = default;
 virtual void blockUpdate(int x, int y, int z) {
  (void)x;
  (void)y;
  (void)z;
 }
 virtual void setBlocksDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
  (void)minX;
  (void)minY;
  (void)minZ;
  (void)maxX;
  (void)maxY;
  (void)maxZ;
 }
 virtual void playSound(const std::string& name, double x, double y, double z, float volume, float pitch) {
  (void)name;
  (void)x;
  (void)y;
  (void)z;
  (void)volume;
  (void)pitch;
 }
 virtual void addParticle(const std::string& name, double x, double y, double z, double vx, double vy, double vz) {
  (void)name;
  (void)x;
  (void)y;
  (void)z;
  (void)vx;
  (void)vy;
  (void)vz;
 }
 virtual void notifyEntityAdded(Entity* entity) {
  (void)entity;
 }
 virtual void notifyEntityRemoved(Entity* entity) {
  (void)entity;
 }
 // A chunk column finished loading/generating and its block data is now
 // readable. Distinct from setBlocksDirty: the renderer only needs to refresh
 // the borders of the four orthogonally adjacent columns, and can coalesce a
 // whole batch of arrivals into one pass instead of re-dirtying a 3x3
 // neighbourhood over full height per chunk.
    virtual void chunkAvailable(int chunkX, int chunkZ) {
     (void)chunkX;
     (void)chunkZ;
    }
    // The column's lighting drained (World::doLightingUpdates); the renderer may
    // now build the first mesh that was held pending lighting.
    virtual void markChunkColumnLit(int chunkX, int chunkZ) {
     (void)chunkX;
     (void)chunkZ;
    }
    // The lighting engine went fully idle; every held column is lit (non-optional
    // completion for columns whose lighting produced no drained region).
    virtual void markAllChunksLit() {
    }
    // A chunk column was unloaded/evicted; its block data is no longer readable.
   // Listeners that mirrored the column (e.g. the chunk renderer) must drop it so
   // a later reload starts fresh instead of reusing stale state.
   virtual void chunkUnloaded(int chunkX, int chunkZ) {
    (void)chunkX;
    (void)chunkZ;
   }
  virtual void notifyAmbientDarknessChanged() {
  }
 virtual void playStreaming(const std::string& name, int x, int y, int z) {
  (void)name;
  (void)x;
  (void)y;
  (void)z;
 }
 virtual void updateBlockEntity(int x, int y, int z, block::entity::BlockEntity* blockEntity) {
  (void)x;
  (void)y;
  (void)z;
  (void)blockEntity;
 }
 virtual void onEntityPickup(Entity* entity, PlayerEntity* collector) {
  (void)entity;
  (void)collector;
 }
 virtual void worldEvent(PlayerEntity* player, int event, int x, int y, int z, int data) {
  (void)player;
  (void)event;
  (void)x;
  (void)y;
  (void)z;
  (void)data;
 }
 virtual void blockBreakParticles(int x, int y, int z, int blockId, int blockMeta) {
  (void)x;
  (void)y;
  (void)z;
  (void)blockId;
  (void)blockMeta;
 }
};
} // namespace net::minecraft
