#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
#include <algorithm>
#include <mutex>
#include "net/minecraft/world/chunk/Chunk.hpp"
namespace net::minecraft::world::light {
UnifiedLightRegistry::ReadView::ReadView(const UnifiedLightRegistry& registry)
    : lock_(registry.mutex_), sources_(&registry.sources_) {}
const PhysicalLight* UnifiedLightRegistry::ReadView::find(const LightKey& key) const noexcept {
  const auto found = std::find_if(sources_->begin(), sources_->end(), [&key](const PhysicalLight& source) {
    return source.key == key;
  });
  return found != sources_->end() ? &*found : nullptr;
}
std::size_t LightKeyHash::operator()(const LightKey& key) const noexcept {
  std::size_t hash = static_cast<std::size_t>(key.domain);
  hash = hash * 1099511628211ULL ^ static_cast<std::uint32_t>(key.x);
  hash = hash * 1099511628211ULL ^ static_cast<std::uint32_t>(key.y);
  hash = hash * 1099511628211ULL ^ static_cast<std::uint32_t>(key.z);
  hash = hash * 1099511628211ULL ^ static_cast<std::size_t>(key.id);
  return hash;
}
LightKey UnifiedLightRegistry::blockKey(int x, int y, int z) noexcept {
  return {LightDomain::Block, x, y, z, 0};
}
LightKey UnifiedLightRegistry::sunKey() noexcept {
  return {LightDomain::Sun, 0, 0, 0, 0};
}
std::uint64_t UnifiedLightRegistry::chunkKey(int chunkX, int chunkZ) noexcept {
  return static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunkX)) << 32U |
         static_cast<std::uint32_t>(chunkZ);
}
void UnifiedLightRegistry::setBlockEmission(int blockId, int emission) noexcept {
  if(blockId >= 0 && static_cast<std::size_t>(blockId) < blockEmission_.size()) {
    blockEmission_[static_cast<std::size_t>(blockId)].store(
        static_cast<std::uint8_t>(std::clamp(emission, 0, 15)), std::memory_order_relaxed);
  }
}
int UnifiedLightRegistry::blockEmission(int blockId) noexcept {
  if(blockId < 0 || static_cast<std::size_t>(blockId) >= blockEmission_.size()) {
    return 0;
  }
  return blockEmission_[static_cast<std::size_t>(blockId)].load(std::memory_order_relaxed);
}
PhysicalLight UnifiedLightRegistry::makeBlockSource(const LightKey& key, int emission) noexcept {
  PhysicalLight source;
  source.key = key;
  source.shape = LightShape::Point;
  source.x = key.x + 0.5;
  source.y = key.y + 0.5;
  source.z = key.z + 0.5;
  source.red = 1.0f;
  source.green = 0.42f;
  source.blue = 0.12f;
  source.radius = 6.0f + static_cast<float>(emission) * 0.6f;
  source.intensity = static_cast<float>(emission) / 15.0f;
  return source;
}
void UnifiedLightRegistry::syncBlockSource(int blockId, int x, int y, int z) {
  const LightKey key = blockKey(x, y, z);
  const int emission = blockEmission(blockId);
  const std::uint64_t owner = chunkKey(x >> 4, z >> 4);
  const std::unique_lock lock(mutex_);
  if(emission <= 0) {
    const bool changed = eraseLocked(key);
    if(const auto chunk = blockSourcesByChunk_.find(owner); chunk != blockSourcesByChunk_.end()) {
      chunk->second.erase(key);
      if(chunk->second.empty()) {
        blockSourcesByChunk_.erase(chunk);
      }
    }
    if(changed) {
      ++revision_;
    }
    return;
  }
  blockSourcesByChunk_[owner].insert(key);
  if(upsertLocked(key, makeBlockSource(key, emission))) {
    ++revision_;
  }
}
bool UnifiedLightRegistry::upsertLocked(const LightKey& key, PhysicalLight source) {
  source.key = key;
  const auto found = indices_.find(key);
  if(found != indices_.end()) {
    PhysicalLight& current = sources_[found->second];
    if(current != source) {
      current = source;
      return true;
    }
    return false;
  }
  indices_[key] = sources_.size();
  sources_.push_back(source);
  return true;
}
bool UnifiedLightRegistry::eraseLocked(const LightKey& key) {
  const auto found = indices_.find(key);
  if(found == indices_.end()) {
    return false;
  }
  const std::size_t index = found->second;
  const std::size_t last = sources_.size() - 1;
  if(index != last) {
    sources_[index] = std::move(sources_[last]);
    indices_[sources_[index].key] = index;
  }
  sources_.pop_back();
  indices_.erase(found);
  return true;
}
void UnifiedLightRegistry::syncChunkSources(const Chunk& chunk) {
  const std::uint64_t owner = chunkKey(chunk.x, chunk.z);
  const std::unique_lock lock(mutex_);
  bool changed = false;
  if(const auto existing = blockSourcesByChunk_.find(owner); existing != blockSourcesByChunk_.end()) {
    for(const LightKey& key : existing->second) {
      changed = eraseLocked(key) || changed;
    }
    blockSourcesByChunk_.erase(existing);
  }
  auto& keys = blockSourcesByChunk_[owner];
  for(int localX = 0; localX < Chunk::width; ++localX) {
    for(int localZ = 0; localZ < Chunk::depth; ++localZ) {
      for(int y = 0; y < Chunk::height; ++y) {
        const int emission = blockEmission(chunk.getBlockId(localX, y, localZ));
        if(emission <= 0) {
          continue;
        }
        const LightKey key = blockKey(chunk.x * Chunk::width + localX, y, chunk.z * Chunk::depth + localZ);
        keys.insert(key);
        changed = upsertLocked(key, makeBlockSource(key, emission)) || changed;
      }
    }
  }
  if(keys.empty()) {
    blockSourcesByChunk_.erase(owner);
  }
  if(changed) {
    ++revision_;
  }
}
void UnifiedLightRegistry::eraseChunkSources(int chunkX, int chunkZ) {
  const std::unique_lock lock(mutex_);
  const auto chunk = blockSourcesByChunk_.find(chunkKey(chunkX, chunkZ));
  if(chunk == blockSourcesByChunk_.end()) {
    return;
  }
  bool changed = false;
  for(const LightKey& key : chunk->second) {
    changed = eraseLocked(key) || changed;
  }
  blockSourcesByChunk_.erase(chunk);
  if(changed) {
    ++revision_;
  }
}
void UnifiedLightRegistry::upsert(const LightKey& key, PhysicalLight source) {
  const std::unique_lock lock(mutex_);
  if(key.domain == LightDomain::Block) {
    blockSourcesByChunk_[chunkKey(key.x >> 4, key.z >> 4)].insert(key);
  }
  if(upsertLocked(key, source)) {
    ++revision_;
  }
}
bool UnifiedLightRegistry::erase(const LightKey& key) {
  const std::unique_lock lock(mutex_);
  if(key.domain == LightDomain::Block) {
    const std::uint64_t owner = chunkKey(key.x >> 4, key.z >> 4);
    if(const auto chunk = blockSourcesByChunk_.find(owner); chunk != blockSourcesByChunk_.end()) {
      chunk->second.erase(key);
      if(chunk->second.empty()) {
        blockSourcesByChunk_.erase(chunk);
      }
    }
  }
  if(!eraseLocked(key)) {
    return false;
  }
  ++revision_;
  return true;
}
void UnifiedLightRegistry::clear() {
  const std::unique_lock lock(mutex_);
  if(!sources_.empty()) {
    sources_.clear();
    indices_.clear();
    blockSourcesByChunk_.clear();
    ++revision_;
  }
}
UnifiedLightRegistry::ReadView UnifiedLightRegistry::read() const {
  return ReadView(*this);
}
}
