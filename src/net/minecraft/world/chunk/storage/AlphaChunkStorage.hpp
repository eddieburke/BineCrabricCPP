#pragma once
#include "net/minecraft/nbt/Nbt.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/storage/ChunkStorage.hpp"
#include <filesystem>
#include <string>
namespace net::minecraft {
namespace fs = std::filesystem;
class AlphaChunkStorage : public ChunkStorage {
public:
  AlphaChunkStorage(fs::path dir, bool make);
  [[nodiscard]] fs::path getChunkFile(int chunkX, int chunkZ) const;
  Chunk loadChunk(World* world, int chunkX, int chunkZ) override;
  void saveChunk(World* world, Chunk& chunk) override;
  // Shared NBT decode path for on-disk Alpha chunks and Region entries that embed Alpha layout.
  static Chunk loadChunkFromRootNbt(World* world, NbtCompound& root, int chunkX, int chunkZ);
  void tick() override {}
  void flush() override {}
  void saveEntities(World* world, Chunk& chunk) override {
    (void)world;
    (void)chunk;
  }

private:
  fs::path dir_;
  bool make_ = false;
};
} // namespace net::minecraft
