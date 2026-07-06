#include "net/minecraft/world/chunk/ClientChunkStream.hpp"
#include "net/minecraft/util/concurrent/FrameBudget.hpp"
#include "net/minecraft/world/chunk/ChunkDecoration.hpp"
#include "net/minecraft/client/gui/screen/LoadingDisplay.hpp"
#include "net/minecraft/world/World.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
namespace net::minecraft {
ClientChunkStream::~ClientChunkStream() {
  genPool_.cancelPending();
  genPool_.drain();
}
ClientChunkStream::ClientChunkStream(World* world, ChunkStorage* storage, ChunkSource* generator)
    : world_(world), empty_(world, 0, 0), generator_(generator), storage_(storage),
      genPool_(util::concurrent::WorkerPool::recommendedThreadCount(2, 2)) {
  // The resident disc grows quadratically with render distance, so let more
  // generation run concurrently; publish (main thread) is the real throttle.
  maxInFlight_ = std::max<std::size_t>(64, static_cast<std::size_t>(genPool_.threadCount()) * 16U);
}
void ClientChunkStream::initAsync(std::uint64_t seed,
                                  std::function<std::unique_ptr<ChunkSource>(std::uint64_t)> genFactory) {
  seed_ = seed;
  genFactory_ = std::move(genFactory);
}
void ClientChunkStream::setCenter(int chunkX, int chunkZ) noexcept {
  if(chunkX == centerChunkX_ && chunkZ == centerChunkZ_) {
    return;
  }
  centerChunkX_ = chunkX;
  centerChunkZ_ = chunkZ;
  resetPrefetchCursor();
}
void ClientChunkStream::setResidentRadius(int radiusChunks) {
  const int radius = std::max(1, radiusChunks);
  if(radius == residentRadiusChunks_) {
    return;
  }
  residentRadiusChunks_ = radius;
  resetPrefetchCursor();
}
bool ClientChunkStream::isWithinResidentRadius(int chunkX, int chunkZ) const noexcept {
  return chunkX >= centerChunkX_ - residentRadiusChunks_ && chunkZ >= centerChunkZ_ - residentRadiusChunks_ &&
         chunkX <= centerChunkX_ + residentRadiusChunks_ && chunkZ <= centerChunkZ_ + residentRadiusChunks_;
}
bool ClientChunkStream::isManagedChunk(const Chunk* chunk) const {
  return chunk != nullptr && chunk != &empty_;
}
Chunk* ClientChunkStream::findLoadedChunk(int chunkX, int chunkZ) const {
  const auto it = residentChunks_.find(ChunkPos{chunkX, chunkZ});
  return it == residentChunks_.end() ? nullptr : it->second;
}
bool ClientChunkStream::isChunkLoaded(int chunkX, int chunkZ) const {
  if(!isWithinResidentRadius(chunkX, chunkZ)) {
    return false;
  }
  if(cachedChunk_ != nullptr && cachedChunkX_ == chunkX && cachedChunkZ_ == chunkZ) {
    return true;
  }
  return residentChunks_.contains(ChunkPos{chunkX, chunkZ});
}
int ClientChunkStream::prefetchPriority(int chunkX, int chunkZ) const noexcept {
  return std::max(std::abs(chunkX - centerChunkX_), std::abs(chunkZ - centerChunkZ_));
}
std::unique_ptr<ChunkSource> ClientChunkStream::acquireGenerator() {
  {
    const std::lock_guard lock(asyncMutex_);
    if(!generatorPool_.empty()) {
      auto generator = std::move(generatorPool_.back());
      generatorPool_.pop_back();
      return generator;
    }
  }
  return genFactory_ ? genFactory_(seed_) : nullptr;
}
void ClientChunkStream::releaseGenerator(std::unique_ptr<ChunkSource> generator) {
  if(generator == nullptr) {
    return;
  }
  const std::lock_guard lock(asyncMutex_);
  generatorPool_.push_back(std::move(generator));
}
bool ClientChunkStream::requestAsync(int chunkX, int chunkZ, int priority) {
  const ChunkPos key{chunkX, chunkZ};
  {
    const std::lock_guard lock(asyncMutex_);
    if(inFlight_.contains(key) || inFlight_.size() >= maxInFlight_) {
      return false;
    }
  }
  auto job = std::make_shared<AsyncJob>();
  job->chunkX = chunkX;
  job->chunkZ = chunkZ;
  {
    const std::lock_guard lock(asyncMutex_);
    inFlight_[key] = job;
  }
  genPool_.submit(
      [this, job, key]() {
        if(storage_ != nullptr) {
          try {
            auto loaded = storage_->loadDetachedChunk(job->chunkX, job->chunkZ);
            auto chunk = std::make_unique<Chunk>(std::move(loaded));
            if(!chunk->empty && chunk->chunkPosEquals(job->chunkX, job->chunkZ)) {
              job->result = std::move(chunk);
            }
          } catch(...) {
          }
        }
        if(job->result == nullptr) {
          if(auto gen = acquireGenerator()) {
            try {
              job->result = std::make_unique<Chunk>(std::move(gen->getChunk(job->chunkX, job->chunkZ)));
              job->result->fill();
            } catch(...) {
              job->failed = true;
            }
            releaseGenerator(std::move(gen));
          }
        }
        if(job->result == nullptr) {
          job->failed = true;
        }
        job->done.store(true, std::memory_order_release);
        const std::lock_guard lock(asyncMutex_);
        completedJobs_.push_back(job);
      },
      priority);
  return true;
}
std::vector<std::shared_ptr<ClientChunkStream::AsyncJob>> ClientChunkStream::pumpCompleted() {
  std::vector<std::shared_ptr<AsyncJob>> done;
  {
    const std::lock_guard lock(asyncMutex_);
    done.swap(completedJobs_);
    for(const auto& job : done) {
      inFlight_.erase(ChunkPos{job->chunkX, job->chunkZ});
    }
  }
  return done;
}
std::unique_ptr<Chunk> ClientChunkStream::tryClaimAsync(int chunkX, int chunkZ) {
  std::shared_ptr<AsyncJob> job;
  {
    const std::lock_guard lock(asyncMutex_);
    const auto it = inFlight_.find(ChunkPos{chunkX, chunkZ});
    if(it == inFlight_.end()) {
      return nullptr;
    }
    job = it->second;
  }
  if(!job->done.load(std::memory_order_acquire)) {
    return nullptr;
  }
  std::unique_ptr<Chunk> result = std::move(job->result);
  {
    const std::lock_guard lock(asyncMutex_);
    inFlight_.erase(ChunkPos{chunkX, chunkZ});
  }
  return result;
}
void ClientChunkStream::resetPrefetchCursor() {
  prefetchRing_ = 0;
  prefetchDx_ = 0;
  prefetchDz_ = -1;
}
bool ClientChunkStream::advancePrefetchCursor(int& outDx, int& outDz) {
  while(prefetchRing_ <= residentRadiusChunks_) {
    while(prefetchDx_ <= prefetchRing_) {
      while(prefetchDz_ < prefetchRing_) {
        ++prefetchDz_;
        if(prefetchRing_ > 0 && std::abs(prefetchDx_) != prefetchRing_ &&
           std::abs(prefetchDz_) != prefetchRing_) {
          continue;
        }
        outDx = prefetchDx_;
        outDz = prefetchDz_;
        return true;
      }
      ++prefetchDx_;
      prefetchDz_ = -prefetchRing_ - 1;
    }
    ++prefetchRing_;
    prefetchDx_ = -prefetchRing_;
    prefetchDz_ = -prefetchRing_ - 1;
  }
  resetPrefetchCursor();
  return false;
}
void ClientChunkStream::prefetch() {
  std::size_t outstanding = 0;
  std::unordered_set<ChunkPos, ChunkPosHash> pending;
  {
    const std::lock_guard lock(asyncMutex_);
    outstanding = inFlight_.size() + readyToPublish_.size();
    pending.reserve(outstanding);
    for(const auto& [pos, job] : inFlight_) {
      (void)job;
      pending.insert(pos);
    }
  }
  for(const auto& job : readyToPublish_) {
    pending.insert(ChunkPos{job->chunkX, job->chunkZ});
  }
  constexpr int kCellsPerPrefetch = 256;
  int cellsBudget = kCellsPerPrefetch;
  int dx = 0;
  int dz = 0;
  while(cellsBudget > 0 && outstanding < maxInFlight_ && advancePrefetchCursor(dx, dz)) {
    --cellsBudget;
    const int chunkX = centerChunkX_ + dx;
    const int chunkZ = centerChunkZ_ + dz;
    const ChunkPos key{chunkX, chunkZ};
    if(!isWithinResidentRadius(chunkX, chunkZ) || isChunkLoaded(chunkX, chunkZ) || pending.contains(key)) {
      continue;
    }
    if(!requestAsync(chunkX, chunkZ, prefetchPriority(chunkX, chunkZ))) {
      continue;
    }
    pending.insert(key);
    ++outstanding;
  }
}
void ClientChunkStream::pumpPublish(std::chrono::steady_clock::duration budget, int minPublish) {
  for(auto& job : pumpCompleted()) {
    if(!job->failed && job->result != nullptr && isWithinResidentRadius(job->chunkX, job->chunkZ)) {
      readyToPublish_.push_back(std::move(job));
    }
  }
  const util::concurrent::FrameBudget frameBudget{
      std::chrono::steady_clock::now() + budget,
      minPublish,
  };
  int published = 0;
  while(!readyToPublish_.empty() && frameBudget.hasRemaining(published)) {
    std::shared_ptr<AsyncJob> job = std::move(readyToPublish_.front());
    readyToPublish_.pop_front();
    const ChunkPos pos{job->chunkX, job->chunkZ};
    if(!isWithinResidentRadius(pos.x, pos.z) || job->result == nullptr || isChunkLoaded(pos.x, pos.z)) {
      continue;
    }
    if(installChunk(pos, std::move(job->result)) != nullptr) {
      queueNeighborDecoration(pos.x, pos.z);
      ++published;
    }
  }
}
void ClientChunkStream::pumpPublishFrame() {
  pumpPublish(std::chrono::microseconds(500), 1);
}
void ClientChunkStream::enqueueDecoration(int chunkX, int chunkZ) {
  const ChunkPos pos{chunkX, chunkZ};
  if(decorationQueued_.insert(pos).second) {
    decorationQueue_.push_back(pos);
  }
}
void ClientChunkStream::pumpDecoration() {
  constexpr int kMinDecorationsPerTick = 2;
  const util::concurrent::FrameBudget frameBudget =
      util::concurrent::FrameBudget::fromMs(2, kMinDecorationsPerTick);
  const std::size_t attempts = decorationQueue_.size();
  std::size_t inspected = 0;
  int decorated = 0;
  while(inspected < attempts && !decorationQueue_.empty() && frameBudget.hasRemaining(decorated)) {
    const ChunkPos pos = decorationQueue_.front();
    decorationQueue_.pop_front();
    ++inspected;
    if(tryDecorate(pos.x, pos.z)) {
      decorationQueued_.erase(pos);
      ++decorated;
      continue;
    }
    if(!isWithinResidentRadius(pos.x, pos.z) || !isChunkLoaded(pos.x, pos.z)) {
      decorationQueued_.erase(pos);
      continue;
    }
    decorationQueue_.push_back(pos);
  }
}
void ClientChunkStream::scheduleEviction() {
  // Nothing leaves the resident square unless the center moved or the radius
  // shrank; skip the O(r^2) border scan on stationary ticks. pumpEviction
  // requeues evictions it could not finish, so the queue is not lost.
  if(centerChunkX_ == lastEvictScanCenterX_ && centerChunkZ_ == lastEvictScanCenterZ_ &&
     residentRadiusChunks_ == lastEvictScanRadius_) {
    return;
  }
  lastEvictScanCenterX_ = centerChunkX_;
  lastEvictScanCenterZ_ = centerChunkZ_;
  lastEvictScanRadius_ = residentRadiusChunks_;
  for(int dz = centerChunkZ_ - residentRadiusChunks_ - 1; dz <= centerChunkZ_ + residentRadiusChunks_ + 1; ++dz) {
    for(int dx = centerChunkX_ - residentRadiusChunks_ - 1; dx <= centerChunkX_ + residentRadiusChunks_ + 1;
        ++dx) {
      if(isWithinResidentRadius(dx, dz) || !isChunkLoaded(dx, dz)) {
        continue;
      }
      const ChunkPos pos{dx, dz};
      if(evictionQueued_.insert(pos).second) {
        evictionQueue_.push_back(pos);
      }
    }
  }
}
void ClientChunkStream::pumpEviction() {
  constexpr int kMaxEvictionsPerTick = 12;
  int evicted = 0;
  const std::size_t attempts = evictionQueue_.size();
  std::size_t inspected = 0;
  while(inspected < attempts && !evictionQueue_.empty() && evicted < kMaxEvictionsPerTick) {
    const ChunkPos pos = evictionQueue_.front();
    evictionQueue_.pop_front();
    evictionQueued_.erase(pos);
    ++inspected;
    if(isWithinResidentRadius(pos.x, pos.z)) {
      continue;
    }
    if(!evictChunk(pos)) {
      if(Chunk* chunk = findLoadedChunk(pos.x, pos.z); isManagedChunk(chunk)) {
        chunk->cancelRenderEviction();
      }
      if(evictionQueued_.insert(pos).second) {
        evictionQueue_.push_back(pos);
      }
      continue;
    }
    ++evicted;
  }
}
std::unique_ptr<Chunk> ClientChunkStream::takeReadyChunk(int chunkX, int chunkZ) {
  for(auto it = readyToPublish_.begin(); it != readyToPublish_.end(); ++it) {
    if((*it)->chunkX != chunkX || (*it)->chunkZ != chunkZ) {
      continue;
    }
    std::unique_ptr<Chunk> chunk = std::move((*it)->result);
    readyToPublish_.erase(it);
    return chunk;
  }
  return nullptr;
}
std::unique_ptr<Chunk> ClientChunkStream::loadFromStorage(const ChunkPos& pos) {
  if(storage_ == nullptr) {
    return nullptr;
  }
  try {
    auto loaded = std::make_unique<Chunk>(std::move(storage_->loadDetachedChunk(pos.x, pos.z)));
    if(loaded->empty || !loaded->chunkPosEquals(pos.x, pos.z)) {
      return nullptr;
    }
    loaded->attachToWorld(world_);
    return loaded;
  } catch(...) {
    return nullptr;
  }
}
std::unique_ptr<Chunk> ClientChunkStream::loadFromGenerator(const ChunkPos& pos) {
  auto generated = std::make_unique<Chunk>(std::move(generator_->getChunk(pos.x, pos.z)));
  generated->attachToWorld(world_);
  generated->fill();
  return generated;
}
void ClientChunkStream::dropOwnedResident(const ChunkPos& pos) {
  const auto it = ownedChunks_.find(pos);
  if(it == ownedChunks_.end()) {
    return;
  }
  if(cachedChunk_ == it->second.get()) {
    cachedChunk_ = nullptr;
  }
  ownedChunks_.erase(it);
}
Chunk* ClientChunkStream::installChunk(const ChunkPos& pos, std::unique_ptr<Chunk> chunk) {
  if(chunk == nullptr) {
    return installEmptyChunk(pos);
  }
  dropOwnedResident(pos);
  chunk->attachToWorld(world_);
  Chunk* raw = chunk.get();
  raw->populateBlockLight();
  raw->load();
  if(!raw->empty) {
    raw->relightSkylightGaps();
  }
  ownedChunks_[pos] = std::move(chunk);
  residentChunks_[pos] = raw;
  return raw;
}
Chunk* ClientChunkStream::installEmptyChunk(const ChunkPos& pos) {
  dropOwnedResident(pos);
  residentChunks_[pos] = &empty_;
  return &empty_;
}
bool ClientChunkStream::evictChunk(const ChunkPos& pos) {
  auto residentIt = residentChunks_.find(pos);
  if(residentIt == residentChunks_.end()) {
    return true;
  }
  Chunk* chunk = residentIt->second;
  if(cachedChunk_ == chunk) {
    cachedChunk_ = nullptr;
  }
  if(isManagedChunk(chunk)) {
    if(!chunk->beginRenderEviction()) {
      return false;
    }
    chunk->unload();
    if(chunk->shouldSave(false)) {
      saveChunk(*chunk);
      chunk->dirty = false;
    }
    if(!chunk->empty) {
      saveEntitiesForChunk(*chunk);
    }
  }
  residentChunks_.erase(residentIt);
  ownedChunks_.erase(pos);
  return true;
}
void ClientChunkStream::saveEntitiesForChunk(Chunk& chunk) {
  if(storage_ == nullptr) {
    return;
  }
  try {
    storage_->saveEntities(world_, chunk);
  } catch(...) {
  }
}
void ClientChunkStream::saveChunk(Chunk& chunk) {
  if(storage_ == nullptr) {
    return;
  }
  try {
    storage_->saveChunk(world_, chunk);
  } catch(...) {
  }
}
bool ClientChunkStream::canDecorate(int chunkX, int chunkZ) const {
  return isChunkLoaded(chunkX + 1, chunkZ + 1) && isChunkLoaded(chunkX, chunkZ + 1) &&
         isChunkLoaded(chunkX + 1, chunkZ);
}
bool ClientChunkStream::tryDecorate(int chunkX, int chunkZ) {
  Chunk* chunk = findLoadedChunk(chunkX, chunkZ);
  if(!isManagedChunk(chunk) || chunk->terrainPopulated) {
    return true;
  }
  if(!canDecorate(chunkX, chunkZ)) {
    return false;
  }
  decorate(generator_, chunkX, chunkZ);
  return true;
}
void ClientChunkStream::decorate(ChunkSource* source, int chunkX, int chunkZ) {
  world::chunk::decoratePopulatedChunk(world_, getChunk(chunkX, chunkZ), source, generator_, chunkX, chunkZ);
}
void ClientChunkStream::enqueueDecorationIfNeeded(int chunkX, int chunkZ) {
  Chunk* chunk = findLoadedChunk(chunkX, chunkZ);
  if(!isManagedChunk(chunk) || chunk->terrainPopulated) {
    return;
  }
  enqueueDecoration(chunkX, chunkZ);
}
void ClientChunkStream::queueNeighborDecoration(int chunkX, int chunkZ) {
  enqueueDecorationIfNeeded(chunkX, chunkZ);
  enqueueDecorationIfNeeded(chunkX - 1, chunkZ);
  enqueueDecorationIfNeeded(chunkX, chunkZ - 1);
  enqueueDecorationIfNeeded(chunkX - 1, chunkZ - 1);
}
Chunk& ClientChunkStream::getChunk(int chunkX, int chunkZ) {
  if(cachedChunk_ != nullptr && cachedChunkX_ == chunkX && cachedChunkZ_ == chunkZ) {
    return *cachedChunk_;
  }
  if(world_ != nullptr && !world_->isEventProcessingEnabled() && !isWithinResidentRadius(chunkX, chunkZ)) {
    return empty_;
  }
  const ChunkPos pos{chunkX, chunkZ};
  if(Chunk* loaded = findLoadedChunk(chunkX, chunkZ)) {
    cachedChunkX_ = chunkX;
    cachedChunkZ_ = chunkZ;
    cachedChunk_ = loaded;
    return *loaded;
  }
  std::unique_ptr<Chunk> chunk = takeReadyChunk(chunkX, chunkZ);
  if(chunk == nullptr) {
    chunk = tryClaimAsync(chunkX, chunkZ);
  }
  if(chunk == nullptr) {
    chunk = loadFromStorage(pos);
  }
  if(chunk == nullptr && generator_ != nullptr) {
    chunk = loadFromGenerator(pos);
  }
  Chunk* installed = chunk != nullptr ? installChunk(pos, std::move(chunk)) : installEmptyChunk(pos);
  queueNeighborDecoration(chunkX, chunkZ);
  cachedChunkX_ = chunkX;
  cachedChunkZ_ = chunkZ;
  cachedChunk_ = installed;
  return *installed;
}
void ClientChunkStream::populateReadyChunks() {
  for(int dz = centerChunkZ_ - residentRadiusChunks_; dz <= centerChunkZ_ + residentRadiusChunks_; ++dz) {
    for(int dx = centerChunkX_ - residentRadiusChunks_; dx <= centerChunkX_ + residentRadiusChunks_; ++dx) {
      enqueueDecorationIfNeeded(dx, dz);
    }
  }
}
bool ClientChunkStream::tick() {
  pumpPublish(std::chrono::milliseconds(3), 4);
  pumpDecoration();
  scheduleEviction();
  pumpEviction();
  if(storage_ != nullptr) {
    storage_->tick();
  }
  return generator_ != nullptr && generator_->tick();
}
void ClientChunkStream::prepareForSave() {
  genPool_.cancelPending();
  {
    const std::lock_guard lock(asyncMutex_);
    inFlight_.clear();
    completedJobs_.clear();
    readyToPublish_.clear();
  }
  decorationQueue_.clear();
  decorationQueued_.clear();
  genPool_.drain();
  {
    const std::lock_guard lock(asyncMutex_);
    completedJobs_.clear();
    readyToPublish_.clear();
    inFlight_.clear();
  }
}
bool ClientChunkStream::save(bool saveEntities, client::gui::screen::LoadingDisplay* display) {
  int total = 0;
  if(display != nullptr) {
    for(const auto& [pos, chunk] : residentChunks_) {
      (void)pos;
      if(isManagedChunk(chunk) && chunk->shouldSave(saveEntities)) {
        ++total;
      }
    }
  }
  int saved = 0;
  int index = 0;
  for(const auto& [pos, chunk] : residentChunks_) {
    (void)pos;
    if(!isManagedChunk(chunk)) {
      continue;
    }
    if(saveEntities && !chunk->empty) {
      saveEntitiesForChunk(*chunk);
    }
    if(!chunk->shouldSave(saveEntities)) {
      continue;
    }
    saveChunk(*chunk);
    chunk->dirty = false;
    if(!saveEntities && ++saved == 2) {
      return false;
    }
    if(display != nullptr && total > 0 && ++index % 10 == 0) {
      display->progressStagePercentage(index * 100 / total);
    }
  }
  if(saveEntities && storage_ != nullptr) {
    storage_->flush();
  }
  return true;
}
std::string ClientChunkStream::debugInfo() const {
  std::size_t inFlight = 0;
  {
    const std::lock_guard lock(asyncMutex_);
    inFlight = inFlight_.size();
  }
  return "ChunkCache: resident=" + std::to_string(residentChunks_.size()) +
         ", radius=" + std::to_string(residentRadiusChunks_) + ", inFlight=" + std::to_string(inFlight) +
         ", publishQ=" + std::to_string(readyToPublish_.size());
}
} // namespace net::minecraft
