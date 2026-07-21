#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "net/minecraft/util/math/noise/OctavePerlinNoiseSampler.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/gen/carver/CaveWorldCarver.hpp"
namespace net::minecraft {
class Biome;
class Chunk;
class World;
class SkyChunkGenerator : public ChunkSource {
 public:
 explicit SkyChunkGenerator(World* world, std::uint64_t seed);
 void useLocalBiomeSource(bool value) noexcept {
  useLocalBiomeSource_ = value;
 }
 Chunk& loadChunk(int chunkX, int chunkZ) override;
 Chunk& getChunk(int chunkX, int chunkZ) override;
 [[nodiscard]] bool isChunkLoaded(int chunkX, int chunkZ) const override;
 void decorate(ChunkSource* source, int chunkX, int chunkZ) override;
 bool save(bool saveEntities, client::gui::screen::LoadingDisplay* display) override;
 bool tick() override;
 [[nodiscard]] bool canSave() const override;
 [[nodiscard]] std::string getDebugInfo() const override;

 private:
 [[nodiscard]] BiomeSource* activeBiomeSource();
 [[nodiscard]] const BiomeSource* activeBiomeSource() const;
 void buildTerrain(int chunkX, int chunkZ, Chunk& chunk, const std::vector<double>& temperatures);
 void buildSurfaces(int chunkX, int chunkZ, Chunk& chunk, const std::vector<Biome*>& biomes);
 std::vector<double>& generateHeightMap(
     std::vector<double>& heightMap, int x, int y, int z, int sizeX, int sizeY, int sizeZ);
 JavaRandom random_;
 std::uint64_t seed_ = 0;
 World* world_ = nullptr;
 bool useLocalBiomeSource_ = false;
 OctavePerlinNoiseSampler minLimitPerlinNoise_;
 OctavePerlinNoiseSampler maxLimitPerlinNoise_;
 OctavePerlinNoiseSampler perlinNoise1_;
 OctavePerlinNoiseSampler perlinNoise2_;
 OctavePerlinNoiseSampler perlinNoise3_;
 OctavePerlinNoiseSampler floatingIslandScale_;
 OctavePerlinNoiseSampler floatingIslandNoise_;
 OctavePerlinNoiseSampler forestNoise_;
 BiomeSource biomeSource_;
 std::vector<double> heightMap_;
 std::vector<Biome*> biomes_;
 std::vector<double> sandBuffer_;
 std::vector<double> gravelBuffer_;
 std::vector<double> depthBuffer_;
 CaveWorldCarver cave_;
 std::vector<double> perlinNoiseBuffer_;
 std::vector<double> minLimitPerlinNoiseBuffer_;
 std::vector<double> maxLimitPerlinNoiseBuffer_;
 std::vector<double> scaleNoiseBuffer_;
 std::vector<double> depthNoiseBuffer_;
 std::vector<double> decorateTemperatures_;
 std::optional<Chunk> scratchChunk_;
};
} // namespace net::minecraft
