#pragma once
#include <cstdint>
#include <vector>
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
 // Write already-serialized Alpha/Region chunk bytes. Default falls back to no-op;
 // Region storage overrides so the tick thread can hand off disk I/O.
 virtual void writeSerializedChunk(int chunkX, int chunkZ, const std::vector<std::uint8_t>& data) {
  (void)chunkX;
  (void)chunkZ;
  (void)data;
 }
 [[nodiscard]] virtual bool supportsAsyncWrites() const noexcept {
  return false;
 }
 virtual void saveEntities(World* world, Chunk& chunk) = 0;
 virtual void tick() = 0;
 virtual void flush() = 0;
};
} // namespace net::minecraft
