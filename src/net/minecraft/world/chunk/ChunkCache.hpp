#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/chunk/EmptyChunk.hpp"
#include "net/minecraft/world/chunk/storage/AlphaChunkStorage.hpp"
#include "net/minecraft/world/chunk/storage/ChunkStorage.hpp"
namespace net::minecraft {
class Chunk;
class World;
namespace world::chunk {
class ChunkCache : public ChunkSource {
 public:
 ChunkCache(World* world, std::unique_ptr<ChunkStorage> storage, ChunkSource* generator);
 ~ChunkCache() override;
 bool forceLoad = false;
 [[nodiscard]] bool isChunkLoaded(int chunkX, int chunkZ) const override;
 [[nodiscard]] bool isChunkDataReady(int chunkX, int chunkZ) const override;
 [[nodiscard]] Chunk* getChunkIfLoaded(int chunkX, int chunkZ) override;
 void dropChunk(int chunkX, int chunkZ);
 Chunk& getChunk(int chunkX, int chunkZ) override;
 Chunk& loadChunk(int chunkX, int chunkZ) override;
 void decorate(ChunkSource* source, int chunkX, int chunkZ) override;
 bool save(bool saveEntityData, SaveProgressCallback progress = nullptr) override;
 bool tick() override;
 void prepareForSave() override;
 [[nodiscard]] bool canSave() const override;
 [[nodiscard]] std::string getDebugInfo() const override;
 void unloadChunk(int chunkX, int chunkZ);
 void setActiveRadius(int radius) override;
 void setChunkCacheCenter(int chunkX, int chunkZ) override;
 void pumpChunkPublish() override;
 void prefetchChunksNear(int centerChunkX, int centerChunkZ) override;
 void forEachLoadedChunk(const LoadedChunkVisitor& visitor) override;
 // Queue a background load/generate for the chunk; the result is folded into
 // the world by tick(). No-op if loaded, pending, or async-incapable.
 void requestChunkAsync(int chunkX, int chunkZ, int priority = 0);
 void markChunkDataReady(int chunkX, int chunkZ) override;
 // Main-thread only: number of async chunk loads not yet integrated.
 [[nodiscard]] std::size_t pendingAsyncLoadCount() const noexcept {
  return pendingLoads_.size();
 }
 [[nodiscard]] std::size_t desiredChunkCount() const noexcept {
  return desiredChunks_.size();
 }
 [[nodiscard]] std::size_t queuedResidencyCount() const noexcept {
  return residencyQueue_.size();
 }
 [[nodiscard]] std::size_t loadedChunkCount() const noexcept {
  return chunksByPos_.size();
 }

 private:
 struct PendingLoad {
  int chunkX = 0;
  int chunkZ = 0;
  std::uint64_t generation = 0;
  std::unique_ptr<Chunk> chunk;
  std::atomic<bool> done{false};
  std::atomic<std::uint64_t> cancelledGeneration{0};
 };
 struct SerializedWrite {
  int chunkX = 0;
  int chunkZ = 0;
  std::vector<std::uint8_t> raw;
  std::unique_ptr<AlphaChunkStorage::ChunkSnapshot> snapshot;
  Chunk* chunk = nullptr;
  std::uint64_t saveTime = 0;
 };
 // Storage/generator work only; safe off-thread.
 std::unique_ptr<Chunk> produceChunk(int chunkX, int chunkZ);
 // Worker-local terrain generator clone (seed-deterministic, local BiomeSource).
 ChunkSource* workerGenerator();
 // Main-thread integration: ownership, maps, light population, load, decorate.
 Chunk& adoptChunk(int chunkX, int chunkZ, std::unique_ptr<Chunk> owned);
 void integrateFinishedLoads(int budget, std::int64_t timeBudgetNs = -1);
 void drainChunksToUnload(int maxChunks);
 void saveEntities(Chunk& chunk);
 void saveChunk(Chunk& chunk);
 void enqueueSnapshotWrite(Chunk* chunk, std::uint64_t saveTime);
 void waitForPendingWrites();
 void drainSerializedWrites();
 void completeSerializedWrite();
 void waitForPendingLoads();
 void rebuildResidencyPlan();
 [[nodiscard]] bool isDesired(const ChunkPos& position) const noexcept;
 // Destroys graveyard entries whose render leases have drained. Tombstone
 // eviction: a chunk is removed from every map and parked here, and is freed
 // only once no worker holds a lease on it (see Chunk::tryAcquireRenderPin).
 void sweepGraveyard();
 EmptyChunk empty_;
 World* world_ = nullptr;
 std::unique_ptr<ChunkStorage> storage_{};
 ChunkSource* generator_ = nullptr;
 std::unordered_map<ChunkPos, Chunk*, ChunkPosHash> chunksByPos_{};
 std::vector<Chunk*> chunks_{};
 std::unordered_map<Chunk*, std::unique_ptr<Chunk>> ownedChunks_{};
 // Evicted-but-leased chunks, kept alive until every worker lease drains.
 std::vector<std::unique_ptr<Chunk>> graveyard_{};
 std::unordered_set<ChunkPos, ChunkPosHash> chunksToUnload_{};
 int activeRadius_ = 15;
 int centerChunkX_ = 0;
 int centerChunkZ_ = 0;
 int plannedCenterChunkX_ = std::numeric_limits<int>::min();
 int plannedCenterChunkZ_ = std::numeric_limits<int>::min();
 int plannedRadius_ = -1;
 std::deque<ChunkPos> residencyQueue_{};
 std::unordered_set<ChunkPos, ChunkPosHash> desiredChunks_{};
 std::unordered_map<ChunkPos, std::shared_ptr<PendingLoad>, ChunkPosHash> pendingLoads_{};
 std::uint64_t nextLoadGeneration_ = 1;
 std::atomic<int> pendingLoadTasks_{0};
 std::mutex loadCompleteMutex_;
 std::condition_variable loadCompleteCv_;
 std::mutex workerGeneratorMutex_;
 std::unordered_map<std::thread::id, std::unique_ptr<ChunkSource>> workerGenerators_{};
 bool pendingIncrementalSave_ = false;
 std::atomic<int> pendingSaveWrites_{0};
 std::mutex saveQueueMutex_;
 std::deque<SerializedWrite> saveQueue_;
 bool saveDrainScheduled_ = false;
 std::mutex saveCompleteMutex_;
 std::condition_variable saveCompleteCv_;
};
} // namespace world::chunk
} // namespace net::minecraft
