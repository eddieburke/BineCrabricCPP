#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace net::minecraft {
class BiomeSource;
class Chunk;
class World;
} // namespace net::minecraft
namespace net::minecraft::mod::lua {
enum class ChunkWriteMode {
 ChunkApi = 0,
 RawGeneration = 1,
};
class LuaChunkContext {
 public:
 class Scope {
public:
  Scope(Chunk* chunk,
        World* world,
        int chunkX,
        int chunkZ,
        ChunkWriteMode mode,
        BiomeSource* biomeSource = nullptr);
  ~Scope();
  Scope(const Scope&) = delete;
  Scope& operator=(const Scope&) = delete;
 private:
  Chunk* previousChunk_;
  World* previousWorld_;
  BiomeSource* previousBiomeSource_;
  int previousChunkX_;
  int previousChunkZ_;
  ChunkWriteMode previousMode_;
 };
 [[nodiscard]] static bool hasActiveChunk() noexcept;
 [[nodiscard]] static Chunk* activeChunk() noexcept;
 [[nodiscard]] static World* activeWorld() noexcept;
 [[nodiscard]] static BiomeSource* activeBiomeSource() noexcept;
 [[nodiscard]] static int activeChunkX() noexcept;
 [[nodiscard]] static int activeChunkZ() noexcept;
 [[nodiscard]] static ChunkWriteMode writeMode() noexcept;
 static bool setBlock(int localX, int y, int localZ, int blockId);
 static bool fillDensity(const double* densityGrid, std::size_t count, int seaLevel = 64);
 [[nodiscard]] static int getBlock(int localX, int y, int localZ);
 [[nodiscard]] static int getHeight(int localX, int localZ);
 [[nodiscard]] static int getTopSolidY(int localX, int localZ);
 [[nodiscard]] static double getTemperature(int localX, int localZ);
 [[nodiscard]] static double getDownfall(int localX, int localZ);
 [[nodiscard]] static std::string_view getBiomeName(int localX, int localZ);
};
} // namespace net::minecraft::mod::lua
