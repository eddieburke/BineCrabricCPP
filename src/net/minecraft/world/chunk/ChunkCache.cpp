#include "net/minecraft/world/chunk/ChunkCache.hpp"

#include <algorithm>

#include "net/minecraft/world/ServerWorld.hpp"

namespace net::minecraft {
bool ChunkCache::tick() {
    if (serverWorld_ == nullptr || !serverWorld_->savingDisabled) {
        for (int i = 0; i < 100; ++i) {
            if (chunksToUnload_.empty()) {
                break;
            }
            const ChunkPos pos = *chunksToUnload_.begin();
            chunksToUnload_.erase(chunksToUnload_.begin());
            const auto mapIt = chunkByPos_.find(pos);
            if (mapIt == chunkByPos_.end()) {
                continue;
            }
            Chunk* chunk = mapIt->second;
            chunk->unload();
            saveChunk(*chunk);
            saveEntitiesForChunk(*chunk);
            chunkByPos_.erase(mapIt);
            chunks_.erase(std::remove(chunks_.begin(), chunks_.end(), chunk), chunks_.end());
        }
        if (storage_ != nullptr) {
            storage_->tick();
        }
    }
    return generator_ != nullptr && generator_->tick();
}

bool ChunkCache::canSave() const {
    if (serverWorld_ != nullptr && serverWorld_->savingDisabled) {
        return false;
    }
    return true;
}
}  // namespace net::minecraft
