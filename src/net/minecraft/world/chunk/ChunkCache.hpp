#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "net/minecraft/client/gui/screen/LoadingDisplay.hpp"
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
 void dropChunk(int chunkX, int chunkZ);
 Chunk& getChunk(int chunkX, int chunkZ) override;
 Chunk& loadChunk(int chunkX, int chunkZ) override;
 void decorate(ChunkSource* source, int chunkX, int chunkZ) override;
 bool save(bool saveEntityData, client::gui::screen::LoadingDisplay* display) override;
 bool tick() override;
 void prepareForSave() override;
 [[nodiscard]] bool canSave() const override;
 [[nodiscard]] std::string getDebugInfo() const override;
 void unloadChunk(int chunkX, int chunkZ);
 void setActiveRadius(int radius) override;
 void setChunkCacheCenter(int chunkX, int chunkZ) override;
 void pumpChunkPublish() override;
 void prefetchChunksNear(int centerChunkX, int centerChunkZ) override;
 // Queue a background load/generate for the chunk; the result is folded into
 // the world by tick(). No-op if loaded, pending, or async-incapable.
 void requestChunkAsync(int chunkX, int chunkZ, int priority = 0);
 void markChunkDataReady(int chunkX, int chunkZ) override;

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
 };
 // Storage/generator work only; safe off-thread.
 std::unique_ptr<Chunk> produceChunk(int chunkX, int chunkZ);
 // Worker-local terrain generator clone (seed-deterministic, local BiomeSource).
 ChunkSource* workerGenerator();
  // Main-thread integration: ownership, maps, light population, load, decorate.
  Chunk& adoptChunk(int chunkX, int chunkZ, std::unique_ptr<Chunk> owned);
  // Integrate up to `budget` finished async loads. When timeBudgetNs > 0, stop
  // as soon as that much wall time elapses too, so a backlog of ready chunks
  // (each of which may run main-thread decoration + block-light population)
  // cannot turn one caller's frame into a multi-second hitch. At least one
  // chunk is always integrated when one is ready.
  void integrateFinishedLoads(int budget, std::int64_t timeBudgetNs = -1);
  void drainChunksToUnload(int maxChunks);
 void saveEntities(Chunk& chunk);
 void saveChunk(Chunk& chunk);
 void enqueueSerializedWrite(int chunkX, int chunkZ, std::vector<std::uint8_t> raw);
 void enqueueSerializedWrite(int chunkX, int chunkZ, AlphaChunkStorage::ChunkSnapshot snapshot);
 void waitForPendingWrites();
 void drainSerializedWrites();
 void completeSerializedWrite();
  void waitForPendingLoads();
  // Waits (bounded) for in-flight render pins to drain before eviction. Returns
  // false if the pins did not drain within the timeout; the caller must keep the
  // chunk loaded and retry later rather than unload a chunk workers may still read.
  static bool retireFromLighting(Chunk* chunk);
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
 std::uint64_t nextLoadGeneration_ = 1;
 std::atomic<int> pendingLoadTasks_{0};
 std::mutex loadCompleteMutex_;
 std::condition_variable loadCompleteCv_;
 // Serializes calls into storage_ only. Decoration, generator_->tick() and the
 // background write drain deliberately run outside it: decoration is main-thread
 // confined, generator_ is main-thread only (workers use workerGenerators_), and
 // every ChunkStorage implementation already locks its own files. Widening this
 // lock again re-creates the convoy where one main-thread decoration pass stalls
 // every compute worker sitting in produceChunk().
 std::recursive_mutex storageMutex_;
 // Per-thread generator clones, owned here so they never outlive world_.
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
