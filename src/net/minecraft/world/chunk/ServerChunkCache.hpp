#pragma once

#include "net/minecraft/world/chunk/ChunkCache.hpp"

namespace net::minecraft {

class ServerWorld;

// Faithful port of net.minecraft.server.world.chunk.ServerChunkCache (beta 1.7.3).
class ServerChunkCache : public ChunkCache {
public:
    ServerChunkCache(ServerWorld* world, std::unique_ptr<ChunkStorage> storage, ChunkSource* generator);

    void isLoaded(int chunkX, int chunkZ)
    {
        markChunkUnloadCandidate(chunkX, chunkZ);
    }
};

} // namespace net::minecraft
