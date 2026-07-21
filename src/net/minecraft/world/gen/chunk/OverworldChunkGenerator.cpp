#include "net/minecraft/world/gen/chunk/OverworldChunkGenerator.hpp"
#include <algorithm>
#include "net/minecraft/block/FallingBlock.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/gen/GenerationApi.hpp"
#include "net/minecraft/world/gen/feature/OverworldFeatureDecorator.hpp"
namespace net::minecraft {
BiomeSource* OverworldChunkGenerator::activeBiomeSource() {
 if(!useLocalBiomeSource_ && world_ != nullptr && world_->getBiomeSource() != nullptr) {
  return world_->getBiomeSource();
 }
 return &biomeSource_;
}
const BiomeSource* OverworldChunkGenerator::activeBiomeSource() const {
 if(!useLocalBiomeSource_ && world_ != nullptr && world_->getBiomeSource() != nullptr) {
  return world_->getBiomeSource();
 }
 return &biomeSource_;
}
Chunk OverworldChunkGenerator::loadChunk(ChunkSource* source, int chunkX, int chunkZ) {
 random_.setSeed(static_cast<std::uint64_t>(static_cast<std::int64_t>(chunkX)) * 341873128712ULL +
                 static_cast<std::uint64_t>(static_cast<std::int64_t>(chunkZ)) * 132897987541ULL);
 Chunk chunk(world_, chunkX, chunkZ);
 BiomeSource* biomeSource = activeBiomeSource();
 biomes_ = biomeSource->getBiomesInArea(biomes_, chunkX * 16, chunkZ * 16, 16, 16);
 world::gen::ChunkGenerationContext context{
     world_,
     source,
     &chunk,
     biomeSource,
     &random_,
     world_ != nullptr ? world_->getSeed() : seed_,
     chunkX,
     chunkZ,
     world_ != nullptr ? world_->isLuaModGenerationEnabled() : modGenerationEnabled_,
     true};
 world::gen::runVanillaStage(world::gen::ChunkStage::Terrain, context, [&] {
  buildTerrain(chunkX, chunkZ, chunk, biomeSource->temperatureMap());
 });
 world::gen::runVanillaStage(
     world::gen::ChunkStage::Surface, context, [&] { buildSurfaces(chunkX, chunkZ, chunk, biomes_); });
 if(source != nullptr) {
  world::gen::runVanillaStage(world::gen::ChunkStage::Carver, context, [&] {
   cave_.place(source, world_, seed_, chunkX, chunkZ, chunk);
  });
 }
 if(world_ != nullptr) {
  chunk.populateHeightMap();
 } else {
  chunk.populateHeightMapOnly();
 }
 return chunk;
}
void OverworldChunkGenerator::decorate(ChunkSource* source, int chunkX, int chunkZ) {
 block::FallingBlock::fallInstantly = true;
 BiomeSource* biomeSource = activeBiomeSource();
 const std::uint64_t worldSeed = world_ != nullptr ? world_->getSeed() : seed_;
 (void)world::gen::seedPopulationRandom(random_, worldSeed, chunkX, chunkZ);
 if(world_ == nullptr || biomeSource == nullptr) {
  block::FallingBlock::fallInstantly = false;
  return;
 }
 Chunk* chunk = world_->hasChunk(chunkX, chunkZ) ? &world_->getChunk(chunkX, chunkZ) : nullptr;
 world::gen::ChunkGenerationContext context{world_,
                                            source,
                                            chunk,
                                            biomeSource,
                                            &random_,
                                            worldSeed,
                                            chunkX,
                                            chunkZ,
                                            world_->isLuaModGenerationEnabled(),
                                            true};
 world::gen::runVanillaStage(world::gen::ChunkStage::Features, context, [&] {
  world::gen::feature::decorateOverworldChunk(
      world_, random_, *biomeSource, forestNoise_, chunkX, chunkZ, decorateTemperatures_);
 });
 block::FallingBlock::fallInstantly = false;
}
std::vector<double>& OverworldChunkGenerator::generateHeightMap(
    std::vector<double>& heightMap, int x, int y, int z, int sizeX, int sizeY, int sizeZ) {
 const std::size_t size = static_cast<std::size_t>(sizeX * sizeY * sizeZ);
 if(heightMap.size() < size) {
  heightMap.assign(size, 0.0);
 }
 constexpr double baseScale = 684.412;
 const BiomeSource* biomeSource = activeBiomeSource();
 const std::vector<double>& temperatureMap = biomeSource->temperatureMap();
 const std::vector<double>& downfallMap = biomeSource->downfallMap();
 scaleNoiseBuffer_ = floatingIslandScale_.create(scaleNoiseBuffer_, x, z, sizeX, sizeZ, 1.121, 1.121, 0.5);
 depthNoiseBuffer_ = floatingIslandNoise_.create(depthNoiseBuffer_, x, z, sizeX, sizeZ, 200.0, 200.0, 0.5);
 perlinNoiseBuffer_ = perlinNoise1_.create(
     perlinNoiseBuffer_, x, y, z, sizeX, sizeY, sizeZ, baseScale / 80.0, baseScale / 160.0, baseScale / 80.0);
 minLimitPerlinNoiseBuffer_ = minLimitPerlinNoise_.create(
     minLimitPerlinNoiseBuffer_, x, y, z, sizeX, sizeY, sizeZ, baseScale, baseScale, baseScale);
 maxLimitPerlinNoiseBuffer_ = maxLimitPerlinNoise_.create(
     maxLimitPerlinNoiseBuffer_, x, y, z, sizeX, sizeY, sizeZ, baseScale, baseScale, baseScale);
 int index = 0;
 int horizontalIndex = 0;
 const int biomeStep = 16 / sizeX;
 for(int ix = 0; ix < sizeX; ++ix) {
  const int biomeX = ix * biomeStep + biomeStep / 2;
  for(int iz = 0; iz < sizeZ; ++iz) {
   const int biomeZ = iz * biomeStep + biomeStep / 2;
   const double temperature = temperatureMap[static_cast<std::size_t>(biomeX * 16 + biomeZ)];
   const double downfallTemperature =
       downfallMap[static_cast<std::size_t>(biomeX * 16 + biomeZ)] * temperature;
   double terrainFactor = 1.0 - downfallTemperature;
   terrainFactor *= terrainFactor;
   terrainFactor *= terrainFactor;
   terrainFactor = 1.0 - terrainFactor;
   double scaleNoise = (scaleNoiseBuffer_[static_cast<std::size_t>(horizontalIndex)] + 256.0) / 512.0;
   scaleNoise = std::min(scaleNoise * terrainFactor, 1.0);
   double depthNoise = depthNoiseBuffer_[static_cast<std::size_t>(horizontalIndex)] / 8000.0;
   if(depthNoise < 0.0) {
    depthNoise = -depthNoise * 0.3;
   }
   depthNoise = depthNoise * 3.0 - 2.0;
   if(depthNoise < 0.0) {
    depthNoise = std::max(depthNoise / 2.0, -1.0);
    depthNoise /= 1.4;
    depthNoise /= 2.0;
    scaleNoise = 0.0;
   } else {
    depthNoise = std::min(depthNoise, 1.0) / 8.0;
   }
   scaleNoise = std::max(scaleNoise, 0.0) + 0.5;
   depthNoise = depthNoise * static_cast<double>(sizeY) / 16.0;
   const double center = static_cast<double>(sizeY) / 2.0 + depthNoise * 4.0;
   ++horizontalIndex;
   for(int iy = 0; iy < sizeY; ++iy) {
    double vertical = (static_cast<double>(iy) - center) * 12.0 / scaleNoise;
    if(vertical < 0.0) {
     vertical *= 4.0;
    }
    const double minLimit = minLimitPerlinNoiseBuffer_[static_cast<std::size_t>(index)] / 512.0;
    const double maxLimit = maxLimitPerlinNoiseBuffer_[static_cast<std::size_t>(index)] / 512.0;
    const double selector = (perlinNoiseBuffer_[static_cast<std::size_t>(index)] / 10.0 + 1.0) / 2.0;
    double density = selector < 0.0
                         ? minLimit
                         : (selector > 1.0 ? maxLimit : minLimit + (maxLimit - minLimit) * selector);
    density -= vertical;
    if(iy > sizeY - 4) {
     const double topFade = static_cast<double>(iy - (sizeY - 4)) / 3.0;
     density = density * (1.0 - topFade) + -10.0 * topFade;
    }
    heightMap[static_cast<std::size_t>(index++)] = density;
   }
  }
 }
 return heightMap;
}
} // namespace net::minecraft
