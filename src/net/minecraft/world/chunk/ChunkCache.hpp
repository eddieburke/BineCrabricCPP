#pragma once
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/chunk/ChunkDecoration.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/chunk/EmptyChunk.hpp"
#include "net/minecraft/world/chunk/storage/ChunkStorage.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "net/minecraft/world/World.hpp"
namespace net::minecraft {
class ServerWorld;
// Faithful port of net.minecraft.world.chunk.ChunkCache (beta 1.7.3).
class ChunkCache : public ChunkSource {
public:
  ChunkCache(World* world, std::unique_ptr<ChunkStorage> storage, ChunkSource* generator,
             ServerWorld* serverWorld = nullptr)
      : empty_(world, 0, 0), world_(world), storage_(std::move(storage)), generator_(generator),
        serverWorld_(serverWorld) {
  }
  bool forceLoad = false;
  [[nodiscard]] bool isChunkLoaded(int chunkX, int chunkZ) const override {
    return chunkByPos_.find(ChunkPos{chunkX, chunkZ}) != chunkByPos_.end();
  }
  Chunk& getChunk(int chunkX, int chunkZ) override {
    const ChunkPos pos{chunkX, chunkZ};
    const auto it = chunkByPos_.find(pos);
    if(it == chunkByPos_.end()) {
      if(serverWorld_ == nullptr || world_->isEventProcessingEnabled() || forceLoad) {
        return loadChunk(chunkX, chunkZ);
      }
      return empty_;
    }
    return *it->second;
  }
  Chunk& loadChunk(int chunkX, int chunkZ) override {
    const ChunkPos pos{chunkX, chunkZ};
    chunksToUnload_.erase(pos);
    const auto existing = chunkByPos_.find(pos);
    if(existing != chunkByPos_.end()) {
      return *existing->second;
    }
    Chunk* chunk = loadChunkFromStorage(chunkX, chunkZ);
    if(chunk == nullptr) {
      if(generator_ == nullptr) {
        chunk = &empty_;
      } else {
        chunk = loadChunkFromGenerator(chunkX, chunkZ);
      }
    }
    chunkByPos_[pos] = chunk;
    chunks_.push_back(chunk);
    if(chunk != &empty_ && chunk != nullptr) {
      chunk->populateBlockLight();
      chunk->load();
    }
    if(chunk != nullptr && !chunk->terrainPopulated && isChunkLoaded(chunkX + 1, chunkZ + 1) &&
       isChunkLoaded(chunkX, chunkZ + 1) && isChunkLoaded(chunkX + 1, chunkZ)) {
      decorate(this, chunkX, chunkZ);
    }
    if(isChunkLoaded(chunkX - 1, chunkZ) && !getChunk(chunkX - 1, chunkZ).terrainPopulated &&
       isChunkLoaded(chunkX - 1, chunkZ + 1) && isChunkLoaded(chunkX, chunkZ + 1) &&
       isChunkLoaded(chunkX - 1, chunkZ)) {
      decorate(this, chunkX - 1, chunkZ);
    }
    if(isChunkLoaded(chunkX, chunkZ - 1) && !getChunk(chunkX, chunkZ - 1).terrainPopulated &&
       isChunkLoaded(chunkX + 1, chunkZ - 1) && isChunkLoaded(chunkX, chunkZ - 1) &&
       isChunkLoaded(chunkX + 1, chunkZ)) {
      decorate(this, chunkX, chunkZ - 1);
    }
    if(isChunkLoaded(chunkX - 1, chunkZ - 1) && !getChunk(chunkX - 1, chunkZ - 1).terrainPopulated &&
       isChunkLoaded(chunkX - 1, chunkZ - 1) && isChunkLoaded(chunkX, chunkZ - 1) &&
       isChunkLoaded(chunkX - 1, chunkZ)) {
      decorate(this, chunkX - 1, chunkZ - 1);
    }
    return *chunk;
  }
  void decorate(ChunkSource* source, int chunkX, int chunkZ) override {
    world::chunk::decoratePopulatedChunk(world_, getChunk(chunkX, chunkZ), source, generator_, chunkX, chunkZ);
  }
  bool save(bool saveEntities, client::gui::screen::LoadingDisplay* display) override {
    (void)display;
    int saved = 0;
    for(Chunk* chunk : chunks_) {
      if(chunk == nullptr) {
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
      if(++saved == 24 && !saveEntities) {
        return false;
      }
    }
    if(saveEntities) {
      if(storage_ == nullptr) {
        return true;
      }
      storage_->flush();
    }
    return true;
  }
  bool tick() override;
  [[nodiscard]] bool canSave() const override;
  [[nodiscard]] std::string getDebugInfo() const override {
    return "ServerChunkCache: " + std::to_string(chunkByPos_.size()) +
           " Drop: " + std::to_string(chunksToUnload_.size());
  }

protected:
  void markChunkUnloadCandidate(int chunkX, int chunkZ) {
    if(serverWorld_ == nullptr || world_ == nullptr) {
      return;
    }
    const Vec3i spawnPos = world_->getSpawnPos();
    const int dx = chunkX * 16 + 8 - spawnPos.x;
    const int dz = chunkZ * 16 + 8 - spawnPos.z;
    constexpr int radius = 128;
    if(dx < -radius || dx > radius || dz < -radius || dz > radius) {
      chunksToUnload_.insert(ChunkPos{chunkX, chunkZ});
    }
  }

private:
  Chunk* loadChunkFromStorage(int chunkX, int chunkZ) {
    if(storage_ == nullptr) {
      return nullptr;
    }
    try {
      std::unique_ptr<Chunk> loaded =
          std::make_unique<Chunk>(std::move(storage_->loadChunk(world_, chunkX, chunkZ)));
      if(loaded->empty || !loaded->chunkPosEquals(chunkX, chunkZ)) {
        return nullptr;
      }
      loaded->world = world_;
      if(world_ != nullptr) {
        loaded->lastSaveTime = static_cast<long long>(world_->getTime());
      }
      Chunk* stored = loaded.get();
      ownedChunks_.push_back(std::move(loaded));
      return stored;
    } catch(...) {
      return nullptr;
    }
  }
  Chunk* loadChunkFromGenerator(int chunkX, int chunkZ) {
    std::unique_ptr<Chunk> generated = std::make_unique<Chunk>(std::move(generator_->getChunk(chunkX, chunkZ)));
    generated->world = world_;
    Chunk* stored = generated.get();
    ownedChunks_.push_back(std::move(generated));
    return stored;
  }
  void saveEntitiesForChunk(Chunk& chunk) {
    if(storage_ == nullptr) {
      return;
    }
    try {
      storage_->saveEntities(world_, chunk);
    } catch(...) {
    }
  }
  void saveChunk(Chunk& chunk) {
    if(storage_ == nullptr) {
      return;
    }
    try {
      if(world_ != nullptr) {
        chunk.lastSaveTime = static_cast<long long>(world_->getTime());
      }
      storage_->saveChunk(world_, chunk);
    } catch(...) {
    }
  }
  EmptyChunk empty_;
  World* world_ = nullptr;
  std::unique_ptr<ChunkStorage> storage_{};
  ChunkSource* generator_ = nullptr;
  ServerWorld* serverWorld_ = nullptr;
  std::unordered_map<ChunkPos, Chunk*, ChunkPosHash> chunkByPos_{};
  std::vector<Chunk*> chunks_{};
  std::vector<std::unique_ptr<Chunk>> ownedChunks_{};
  std::unordered_set<ChunkPos, ChunkPosHash> chunksToUnload_{};
};
} // namespace net::minecraft
