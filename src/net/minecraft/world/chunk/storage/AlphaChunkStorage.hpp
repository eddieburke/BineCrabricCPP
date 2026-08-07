#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include "net/minecraft/nbt/Nbt.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/nbt/NbtList.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/chunk/storage/ChunkStorage.hpp"
namespace net::minecraft {
namespace fs = std::filesystem;
// Alpha/Beta per-chunk .dat layout plus the shared Level NBT codec used by McRegion embeds.
class AlphaChunkStorage : public ChunkStorage {
 public:
 struct ChunkSnapshot {
  int x = 0;
  int z = 0;
  std::uint64_t lastUpdate = 0;
  std::vector<std::uint8_t> blocks;
  ChunkNibbleArray meta;
  ChunkNibbleArray skyLight;
  ChunkNibbleArray blockLight;
  std::vector<std::uint8_t> heightmap;
  bool terrainPopulated = false;
  NbtList entities;
  NbtList tileEntities;
 };
 AlphaChunkStorage(fs::path dir, bool make);
 [[nodiscard]] fs::path getChunkFile(int chunkX, int chunkZ) const;
 Chunk loadChunk(World* world, int chunkX, int chunkZ) override;
 void saveChunk(World* world, Chunk& chunk) override;
 // Shared NBT decode path for on-disk Alpha chunks and Region entries that embed Alpha layout.
 static Chunk loadChunkFromRootNbt(World* world, NbtCompound& root, int chunkX, int chunkZ);
  // Direct binary NBT writer for Alpha/Beta chunk "Level" payloads. Skips the variant tree for
  // bulk byte arrays while keeping wire-format parity with Java NbtIo.
  static void writeRootChunk(std::vector<std::uint8_t>& out, Chunk& chunk, World* world);
  // Read-only capture of a chunk's bulk arrays plus entity/block-entity NBT.
  // Never mutates the chunk, so an IO worker may run it against a leased chunk
  // while the main thread keeps mutating world state; entity/block-entity
  // containers are guarded by Chunk::entityMutex_.
  static ChunkSnapshot takeSnapshot(const Chunk& chunk, std::uint64_t lastUpdate);
  static void writeRootChunkFromSnapshot(std::vector<std::uint8_t>& out, const ChunkSnapshot& snapshot);
 // Moves bulk arrays out of parsed NBT when possible to avoid extra copies on load.
 static Chunk loadChunkFromNbt(World* world, NbtCompound& nbt);
 void tick() override {
 }
 void flush() override {
 }
 void saveEntities(World* world, Chunk& chunk) override {
  (void)world;
  (void)chunk;
 }

 private:
 fs::path dir_;
 bool make_ = false;
};
} // namespace net::minecraft
