#include "net/minecraft/world/chunk/ServerChunkCache.hpp"

#include "net/minecraft/world/ServerWorld.hpp"

namespace net::minecraft {

ServerChunkCache::ServerChunkCache(ServerWorld* world, std::unique_ptr<ChunkStorage> storage, ChunkSource* generator)
    : ChunkCache(world, std::move(storage), generator, world)
{
}

} // namespace net::minecraft
