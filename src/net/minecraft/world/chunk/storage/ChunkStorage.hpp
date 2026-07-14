#pragma once
#include "net/minecraft/world/chunk/Chunk.hpp"
namespace net::minecraft {
class World;
class ChunkStorage {
public:
  virtual ~ChunkStorage() = default;
  [[nodiscard]] virtual Chunk loadChunk(World* world, int chunkX, int chunkZ) = 0;
  [[nodiscard]] Chunk loadDetachedChunk(int chunkX, int chunkZ) {
    return loadChunk(nullptr, chunkX, chunkZ);
  }
  virtual void saveChunk(World* world, Chunk& chunk) = 0;
  virtual void saveEntities(World* world, Chunk& chunk) = 0;
  virtual void tick() = 0;
  virtual void flush() = 0;
};
} // namespace net::minecraft
