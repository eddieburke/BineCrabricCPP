#pragma once
#include "net/minecraft/util/concurrent/WorkerPool.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/chunk/EmptyChunk.hpp"
#include "net/minecraft/world/chunk/storage/ChunkStorage.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace net::minecraft {
class World;
// Client-side chunk residency + threaded load/generate pipeline. Workers build
// detached Chunk objects; the main thread publishes, decorates, and evicts.
namespace client::gui::screen {
class LoadingDisplay;
}
class ClientChunkStream {
public:
  ClientChunkStream(World* world, ChunkStorage* storage, ChunkSource* generator);
  ~ClientChunkStream();
  void initAsync(std::uint64_t seed, std::function<std::unique_ptr<ChunkSource>(std::uint64_t)> genFactory);
  void setCenter(int chunkX, int chunkZ) noexcept;
  void setResidentRadius(int radiusChunks);
  [[nodiscard]] bool isWithinResidentRadius(int chunkX, int chunkZ) const noexcept;
  [[nodiscard]] bool isChunkLoaded(int chunkX, int chunkZ) const;
  [[nodiscard]] Chunk* findLoadedChunk(int chunkX, int chunkZ) const;
  Chunk& getChunk(int chunkX, int chunkZ);
  void prefetch();
  void pumpPublishFrame();
  void populateReadyChunks();
  bool tick();
  void decorate(ChunkSource* source, int chunkX, int chunkZ);
  bool save(bool saveEntities, client::gui::screen::LoadingDisplay* display);
  void prepareForSave();
  [[nodiscard]] std::string debugInfo() const;

private:
  struct AsyncJob {
    int chunkX = 0;
    int chunkZ = 0;
    std::unique_ptr<Chunk> result;
    bool failed = false;
    std::atomic<bool> done{false};
  };
  [[nodiscard]] int prefetchPriority(int chunkX, int chunkZ) const noexcept;
  [[nodiscard]] std::unique_ptr<Chunk> loadFromStorage(const ChunkPos& pos);
  [[nodiscard]] std::unique_ptr<Chunk> loadFromGenerator(const ChunkPos& pos);
  void dropOwnedResident(const ChunkPos& pos);
  Chunk* installChunk(const ChunkPos& pos, std::unique_ptr<Chunk> chunk);
  Chunk* installEmptyChunk(const ChunkPos& pos);
  [[nodiscard]] bool evictChunk(const ChunkPos& pos);
  [[nodiscard]] bool canDecorate(int chunkX, int chunkZ) const;
  [[nodiscard]] bool tryDecorate(int chunkX, int chunkZ);
  void enqueueDecorationIfNeeded(int chunkX, int chunkZ);
  void queueNeighborDecoration(int chunkX, int chunkZ);
  [[nodiscard]] bool isManagedChunk(const Chunk* chunk) const;
  [[nodiscard]] std::unique_ptr<Chunk> takeReadyChunk(int chunkX, int chunkZ);
  [[nodiscard]] std::unique_ptr<Chunk> tryClaimAsync(int chunkX, int chunkZ);
  void resetPrefetchCursor();
  [[nodiscard]] bool advancePrefetchCursor(int& outDx, int& outDz);
  void pumpPublish(std::chrono::steady_clock::duration budget, int minPublish);
  void pumpDecoration();
  void scheduleEviction();
  void pumpEviction();
  void enqueueDecoration(int chunkX, int chunkZ);
  bool requestAsync(int chunkX, int chunkZ, int priority);
  std::vector<std::shared_ptr<AsyncJob>> pumpCompleted();
  [[nodiscard]] std::unique_ptr<ChunkSource> acquireGenerator();
  void releaseGenerator(std::unique_ptr<ChunkSource> generator);
  void saveChunk(Chunk& chunk);
  void saveEntitiesForChunk(Chunk& chunk);
  World* world_ = nullptr;
  EmptyChunk empty_;
  ChunkSource* generator_ = nullptr;
  ChunkStorage* storage_ = nullptr;
  util::concurrent::WorkerPool genPool_;
  std::uint64_t seed_ = 0;
  std::function<std::unique_ptr<ChunkSource>(std::uint64_t)> genFactory_;
  std::vector<std::unique_ptr<ChunkSource>> generatorPool_;
  std::size_t maxInFlight_ = 16;
  mutable std::mutex asyncMutex_;
  std::unordered_map<ChunkPos, std::shared_ptr<AsyncJob>, ChunkPosHash> inFlight_;
  std::vector<std::shared_ptr<AsyncJob>> completedJobs_;
  std::deque<std::shared_ptr<AsyncJob>> readyToPublish_;
  std::deque<ChunkPos> decorationQueue_;
  std::unordered_set<ChunkPos, ChunkPosHash> decorationQueued_;
  std::deque<ChunkPos> evictionQueue_;
  std::unordered_set<ChunkPos, ChunkPosHash> evictionQueued_;
  int centerChunkX_ = 0;
  int centerChunkZ_ = 0;
  int residentRadiusChunks_ = 15;
  // Last center/radius scheduleEviction scanned. Chunks only leave range when
  // the center moves or the radius shrinks, so the O(r^2) border scan is
  // skipped on stationary ticks (radius -1 forces the first scan).
  int lastEvictScanCenterX_ = 0;
  int lastEvictScanCenterZ_ = 0;
  int lastEvictScanRadius_ = -1;
  // Incremental prefetch ring cursor (avoids O(r^2) full-disc rescan each tick).
  int prefetchRing_ = 0;
  int prefetchDx_ = 0;
  int prefetchDz_ = -1;
  std::unordered_map<ChunkPos, Chunk*, ChunkPosHash> residentChunks_;
  std::unordered_map<ChunkPos, std::unique_ptr<Chunk>, ChunkPosHash> ownedChunks_;
  Chunk* cachedChunk_ = nullptr;
  int cachedChunkX_ = 0;
  int cachedChunkZ_ = 0;
};
} // namespace net::minecraft
