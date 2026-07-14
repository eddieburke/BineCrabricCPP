#include "net/minecraft/world/chunk/ChunkCache.hpp"
#include <algorithm>
namespace net::minecraft {
bool ChunkCache::tick() {
  {
    for(int i = 0; i < 100; ++i) {
      if(chunksToUnload_.empty()) {
        break;
      }
      const ChunkPos pos = *chunksToUnload_.begin();
      chunksToUnload_.erase(chunksToUnload_.begin());
      const auto mapIt = chunkByPos_.find(pos);
      if(mapIt == chunkByPos_.end()) {
        continue;
      }
      Chunk* chunk = mapIt->second;
      chunk->unload();
      saveChunk(*chunk);
      saveEntitiesForChunk(*chunk);
      chunkByPos_.erase(mapIt);
      chunks_.erase(std::remove(chunks_.begin(), chunks_.end(), chunk), chunks_.end());
    }
    if(storage_ != nullptr) {
      storage_->tick();
    }
  }
  return generator_ != nullptr && generator_->tick();
}
bool ChunkCache::canSave() const {
  return true;
}
} // namespace net::minecraft
