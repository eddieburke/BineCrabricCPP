#include "net/minecraft/world/chunk/ChunkDecoration.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
namespace net::minecraft::world::chunk {
void decoratePopulatedChunk(
    World* world, Chunk& chunk, ChunkSource* source, ChunkSource* generator, int chunkX, int chunkZ) {
 if(chunk.terrainPopulated) {
  return;
 }
 chunk.terrainPopulated = true;
 if(generator != nullptr) {
  generator->decorate(source, chunkX, chunkZ);
 }
 chunk.markDirty();
 if(world != nullptr) {
  world->setBlocksDirty(chunkX * 16, 0, chunkZ * 16, chunkX * 16 + 15, Chunk::height - 1, chunkZ * 16 + 15);
 }
}
} // namespace net::minecraft::world::chunk
