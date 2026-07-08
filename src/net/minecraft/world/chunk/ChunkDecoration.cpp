#include "net/minecraft/world/chunk/ChunkDecoration.hpp"

#include "net/minecraft/mod/GameHooks.hpp"
#include "net/minecraft/mod/HookBus.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"

namespace net::minecraft::world::chunk {
void decoratePopulatedChunk(
    World* world, Chunk& chunk, ChunkSource* source, ChunkSource* generator, int chunkX, int chunkZ) {
    if (chunk.terrainPopulated) {
        return;
    }
    mod::ChunkDecorateEvent beforeEvent{world, chunkX, chunkZ, true, false, &chunk, source};
    mod::hooks().publish(beforeEvent);
    chunk.terrainPopulated = true;
    if (beforeEvent.canceled) {
        chunk.markDirty();
        if (world != nullptr) {
            world->setBlocksDirty(chunkX * 16, 0, chunkZ * 16, chunkX * 16 + 15, Chunk::height - 1, chunkZ * 16 + 15);
        }
        return;
    }
    if (generator != nullptr) {
        generator->decorate(source, chunkX, chunkZ);
    }
    mod::ChunkDecorateEvent afterEvent{world, chunkX, chunkZ, false, false, &chunk, source};
    mod::hooks().publish(afterEvent);
    chunk.markDirty();
    if (world != nullptr) {
        world->setBlocksDirty(chunkX * 16, 0, chunkZ * 16, chunkX * 16 + 15, Chunk::height - 1, chunkZ * 16 + 15);
    }
}
}  // namespace net::minecraft::world::chunk
