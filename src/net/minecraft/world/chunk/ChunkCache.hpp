#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "net/minecraft/util/concurrent/WorkerPool.hpp"
#include "net/minecraft/client/gui/screen/LoadingDisplay.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/chunk/EmptyChunk.hpp"
#include "net/minecraft/world/chunk/storage/ChunkStorage.hpp"
namespace net::minecraft {
class Chunk;
class World;
namespace world::chunk {
class ChunkCache : public ChunkSource {
 public:
 ChunkCache(World* world, std::unique_ptr<ChunkStorage> storage, ChunkSource* generator);
 bool forceLoad = false;
 [[nodiscard]] bool isChunkLoaded(int chunkX, int chunkZ) const override;
 [[nodiscard]] bool isChunkDataReady(int chunkX, int chunkZ) const override;
 void dropChunk(int chunkX, int chunkZ);
 Chunk& getChunk(int chunkX, int chunkZ) override;
 Chunk& loadChunk(int chunkX, int chunkZ) override;
 void decorate(ChunkSource* source, int chunkX, int chunkZ) override;
 bool save(bool saveEntityData, client::gui::screen::LoadingDisplay* display) override;
 bool tick() override;
 void pumpChunkPublish() override;
 [[nodiscard]] bool canSave() const override;
 [[nodiscard]] std::string getDebugInfo() const override;
 void unloadChunk(int chunkX, int chunkZ);
 void setActiveRadius(int radius) override;
 void setChunkCacheCenter(int chunkX, int chunkZ) override;
 void prefetchChunksNear(int centerChunkX, int centerChunkZ) override;
 // Queue a background load/generate for the chunk; the result is folded into
 // the world by tick(). No-op if loaded, pending, or async-incapable.
 void requestChunkAsync(int chunkX, int chunkZ);
 void markChunkDataReady(int chunkX, int chunkZ) override;

 private:
 struct PendingLoad {
  int chunkX = 0;
  int chunkZ = 0;
  std::unique_ptr<Chunk> chunk;
  std::atomic<bool> done{false};
 };
 // Storage/generator work only; safe off-thread under ioMutex_.
 std::unique_ptr<Chunk> produceChunk(int chunkX, int chunkZ);
 // Main-thread integration: ownership, maps, light population, load, decorate.
 Chunk& adoptChunk(int chunkX, int chunkZ, std::unique_ptr<Chunk> owned);
 void integrateFinishedLoads(int budget);
 void saveEntities(Chunk& chunk);
 void saveChunk(Chunk& chunk);
 static void retireFromLighting(Chunk* chunk);
 EmptyChunk empty_;
 World* world_ = nullptr;
 std::unique_ptr<ChunkStorage> storage_{};
 ChunkSource* generator_ = nullptr;
 std::unordered_map<ChunkPos, Chunk*, ChunkPosHash> chunksByPos_{};
 std::vector<Chunk*> chunks_{};
 std::unordered_map<Chunk*, std::unique_ptr<Chunk>> ownedChunks_{};
 std::unordered_set<ChunkPos, ChunkPosHash> chunksToUnload_{};
 int activeRadius_ = 15;
 int centerChunkX_ = 0;
 int centerChunkZ_ = 0;
 std::unordered_map<ChunkPos, std::shared_ptr<PendingLoad>, ChunkPosHash> pendingLoads_{};
 // Serializes storage_/generator_ access between the loader thread and the
 // main thread (saves, decoration, synchronous demand loads). Recursive so
 // decoration that demand-loads a neighbor can re-enter produceChunk.
 std::recursive_mutex ioMutex_;
 // Declared last: destroyed first, so in-flight loads finish before
 // storage_/generator_ go away.
 std::unique_ptr<net::minecraft::util::concurrent::WorkerPool> loaderPool_{};
};
} // namespace world::chunk
} // namespace net::minecraft
