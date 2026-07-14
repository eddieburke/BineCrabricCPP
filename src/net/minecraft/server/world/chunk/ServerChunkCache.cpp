#include "net/minecraft/server/world/chunk/ServerChunkCache.hpp"
#include <algorithm>
#include "net/minecraft/world/ServerWorld.hpp"
namespace net::minecraft::server::world::chunk {
ServerChunkCache::ServerChunkCache(ServerWorld* world, std::unique_ptr<ChunkStorage> storage, ChunkSource* generator)
    : empty_(world, 0, 0), world_(world), storage_(std::move(storage)), generator_(generator) {
}
bool ServerChunkCache::isChunkLoaded(int chunkX, int chunkZ) const {
  return chunksByPos_.find(ChunkPos{chunkX, chunkZ}) != chunksByPos_.end();
}
void ServerChunkCache::isLoaded(int chunkX, int chunkZ) {
  if(world_ == nullptr) {
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
Chunk& ServerChunkCache::loadChunk(int chunkX, int chunkZ) {
  const ChunkPos pos{chunkX, chunkZ};
  chunksToUnload_.erase(pos);
  const auto existing = chunksByPos_.find(pos);
  if(existing != chunksByPos_.end()) {
    return *existing->second;
  }
  Chunk* chunk = loadChunkFromStorage(chunkX, chunkZ);
  if(chunk == nullptr) {
    if(generator_ == nullptr) {
      chunk = &empty_;
    } else {
      std::unique_ptr<Chunk> generated = std::make_unique<Chunk>(std::move(generator_->getChunk(chunkX, chunkZ)));
      generated->world = world_;
      chunk = generated.get();
      ownedChunks_.push_back(std::move(generated));
    }
  }
  chunksByPos_[pos] = chunk;
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
Chunk& ServerChunkCache::getChunk(int chunkX, int chunkZ) {
  const auto it = chunksByPos_.find(ChunkPos{chunkX, chunkZ});
  if(it == chunksByPos_.end()) {
    if(world_ != nullptr && (world_->isEventProcessingEnabled() || forceLoad)) {
      return loadChunk(chunkX, chunkZ);
    }
    return empty_;
  }
  return *it->second;
}
Chunk* ServerChunkCache::loadChunkFromStorage(int chunkX, int chunkZ) {
  if(storage_ == nullptr || world_ == nullptr) {
    return nullptr;
  }
  try {
    std::unique_ptr<Chunk> loaded = std::make_unique<Chunk>(std::move(storage_->loadChunk(world_, chunkX, chunkZ)));
    if(loaded->empty || !loaded->chunkPosEquals(chunkX, chunkZ)) {
      return nullptr;
    }
    loaded->world = world_;
    loaded->lastSaveTime = static_cast<long long>(world_->getTime());
    Chunk* stored = loaded.get();
    ownedChunks_.push_back(std::move(loaded));
    return stored;
  } catch(...) {
    return nullptr;
  }
}
void ServerChunkCache::saveEntities(Chunk& chunk) {
  if(storage_ == nullptr || world_ == nullptr) {
    return;
  }
  try {
    storage_->saveEntities(world_, chunk);
  } catch(...) {
  }
}
void ServerChunkCache::saveChunk(Chunk& chunk) {
  if(storage_ == nullptr || world_ == nullptr) {
    return;
  }
  try {
    chunk.lastSaveTime = static_cast<long long>(world_->getTime());
    storage_->saveChunk(world_, chunk);
  } catch(...) {
  }
}
void ServerChunkCache::decorate(ChunkSource* source, int chunkX, int chunkZ) {
  Chunk& chunk = getChunk(chunkX, chunkZ);
  if(chunk.terrainPopulated) {
    return;
  }
  chunk.terrainPopulated = true;
  if(generator_ != nullptr) {
    generator_->decorate(source, chunkX, chunkZ);
    chunk.markDirty();
  }
}
bool ServerChunkCache::save(bool saveEntityData, client::gui::screen::LoadingDisplay* display) {
  (void)display;
  int saved = 0;
  for(Chunk* chunk : chunks_) {
    if(chunk == nullptr) {
      continue;
    }
    if(saveEntityData && !chunk->empty) {
      saveEntities(*chunk);
    }
    if(!chunk->shouldSave(saveEntityData)) {
      continue;
    }
    saveChunk(*chunk);
    chunk->dirty = false;
    if(++saved == 24 && !saveEntityData) {
      if(storage_ != nullptr) {
        storage_->flush();
      }
      return false;
    }
  }
  if(storage_ != nullptr && (saveEntityData || saved > 0)) {
    storage_->flush();
  }
  return true;
}
bool ServerChunkCache::tick() {
  if(world_ != nullptr && !world_->savingDisabled) {
    for(int i = 0; i < 100; ++i) {
      if(chunksToUnload_.empty()) {
        break;
      }
      const ChunkPos pos = *chunksToUnload_.begin();
      chunksToUnload_.erase(chunksToUnload_.begin());
      const auto mapIt = chunksByPos_.find(pos);
      if(mapIt == chunksByPos_.end()) {
        continue;
      }
      Chunk* chunk = mapIt->second;
      chunk->unload();
      saveChunk(*chunk);
      saveEntities(*chunk);
      chunksByPos_.erase(mapIt);
      chunks_.erase(std::remove(chunks_.begin(), chunks_.end(), chunk), chunks_.end());
    }
    if(storage_ != nullptr) {
      storage_->tick();
    }
  }
  return generator_ != nullptr && generator_->tick();
}
bool ServerChunkCache::canSave() const {
  return world_ == nullptr || !world_->savingDisabled;
}
std::string ServerChunkCache::getDebugInfo() const {
  return "ServerChunkCache: " + std::to_string(chunksByPos_.size()) +
         " Drop: " + std::to_string(chunksToUnload_.size());
}
} // namespace net::minecraft::server::world::chunk
