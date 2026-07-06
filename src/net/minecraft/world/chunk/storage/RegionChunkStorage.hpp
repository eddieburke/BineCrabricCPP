#pragma once
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/storage/ChunkStorage.hpp"
#include <filesystem>
namespace net::minecraft {
namespace fs = std::filesystem;
class World;
class RegionChunkStorage : public ChunkStorage {
public:
  explicit RegionChunkStorage(fs::path dir);
  Chunk loadChunk(World* world, int chunkX, int chunkZ) override;
  void saveChunk(World* world, Chunk& chunk) override;
  void saveEntities(World* world, Chunk& chunk) override;
  void tick() override;
  void flush() override;

private:
  fs::path dir_;
};
} // namespace net::minecraft
