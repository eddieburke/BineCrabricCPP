#pragma once
namespace net::minecraft {
class Chunk;
class ChunkSource;
class World;
namespace world::chunk {
void decoratePopulatedChunk(
    World* world, Chunk& chunk, ChunkSource* source, ChunkSource* generator, int chunkX, int chunkZ);
}
} // namespace net::minecraft
