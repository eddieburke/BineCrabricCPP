#include "net/minecraft/world/gen/chunk/SkyChunkGenerator.hpp"

#include <algorithm>

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/FallingBlock.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/gen/GenerationApi.hpp"
#include "net/minecraft/world/gen/Generator.hpp"
#include "net/minecraft/world/gen/feature/SkyFeatureDecorator.hpp"

namespace net::minecraft {
SkyChunkGenerator::SkyChunkGenerator(World* world, std::uint64_t seed)
    : random_(seed),
      seed_(seed),
      world_(world),
      minLimitPerlinNoise_(random_, 16),
      maxLimitPerlinNoise_(random_, 16),
      perlinNoise1_(random_, 8),
      perlinNoise2_(random_, 4),
      perlinNoise3_(random_, 4),
      floatingIslandScale_(random_, 10),
      floatingIslandNoise_(random_, 16),
      forestNoise_(random_, 8),
      biomeSource_(seed) {
}

BiomeSource* SkyChunkGenerator::activeBiomeSource() {
    if (!useLocalBiomeSource_ && world_ != nullptr && world_->getBiomeSource() != nullptr) {
        return world_->getBiomeSource();
    }
    return &biomeSource_;
}

const BiomeSource* SkyChunkGenerator::activeBiomeSource() const {
    if (!useLocalBiomeSource_ && world_ != nullptr && world_->getBiomeSource() != nullptr) {
        return world_->getBiomeSource();
    }
    return &biomeSource_;
}

Chunk& SkyChunkGenerator::loadChunk(int chunkX, int chunkZ) {
    return getChunk(chunkX, chunkZ);
}

Chunk& SkyChunkGenerator::getChunk(int chunkX, int chunkZ) {
    random_.setSeed(static_cast<std::uint64_t>(static_cast<std::int64_t>(chunkX)) * 341873128712ULL +
                    static_cast<std::uint64_t>(static_cast<std::int64_t>(chunkZ)) * 132897987541ULL);
    Chunk chunk(world_, chunkX, chunkZ);
    BiomeSource* biomeSource = activeBiomeSource();
    biomes_ = biomeSource->getBiomesInArea(biomes_, chunkX * 16, chunkZ * 16, 16, 16);
    world::gen::ChunkGenerationContext context{world_,
                                               this,
                                               &chunk,
                                               biomeSource,
                                               &random_,
                                               world_ != nullptr ? world_->getSeed() : seed_,
                                               chunkX,
                                               chunkZ,
                                               world_ != nullptr && world_->isLuaModGenerationEnabled(),
                                               false};
    world::gen::runVanillaStage(world::gen::ChunkStage::Terrain, context, [&] {
        buildTerrain(chunkX, chunkZ, chunk, biomeSource->temperatureMap());
    });
    world::gen::runVanillaStage(
        world::gen::ChunkStage::Surface, context, [&] { buildSurfaces(chunkX, chunkZ, chunk, biomes_); });
    world::gen::runVanillaStage(
        world::gen::ChunkStage::Carver, context, [&] { cave_.place(this, world_, seed_, chunkX, chunkZ, chunk); });
    if (world_ != nullptr) {
        chunk.populateHeightMap();
    }
    scratchChunk_.emplace(std::move(chunk));
    return *scratchChunk_;
}

bool SkyChunkGenerator::isChunkLoaded(int /*chunkX*/, int /*chunkZ*/) const {
    return true;
}

void SkyChunkGenerator::decorate(ChunkSource* source, int chunkX, int chunkZ) {
    block::FallingBlock::fallInstantly = true;
    BiomeSource* biomeSource = activeBiomeSource();
    const std::uint64_t worldSeed = world_ != nullptr ? world_->getSeed() : seed_;
    (void) world::gen::seedPopulationRandom(random_, worldSeed, chunkX, chunkZ);
    Chunk* chunk = world_ != nullptr && world_->hasChunk(chunkX, chunkZ) ? &world_->getChunk(chunkX, chunkZ) : nullptr;
    world::gen::ChunkGenerationContext context{world_,
                                               source,
                                               chunk,
                                               biomeSource,
                                               &random_,
                                               worldSeed,
                                               chunkX,
                                               chunkZ,
                                               world_ != nullptr && world_->isLuaModGenerationEnabled(),
                                               false};
    world::gen::runVanillaStage(world::gen::ChunkStage::Features, context, [&] {
        world::gen::feature::decorateSkyChunk(
            world_, random_, *biomeSource, forestNoise_, chunkX, chunkZ, decorateTemperatures_);
    });
    block::FallingBlock::fallInstantly = false;
}

bool SkyChunkGenerator::save(bool /*saveEntities*/, client::gui::screen::LoadingDisplay* /*display*/) {
    return true;
}

bool SkyChunkGenerator::tick() {
    return false;
}

bool SkyChunkGenerator::canSave() const {
    return true;
}

std::string SkyChunkGenerator::getDebugInfo() const {
    return "RandomLevelSource";
}

void SkyChunkGenerator::buildTerrain(int chunkX,
                                     int chunkZ,
                                     Chunk& chunk,
                                     const std::vector<double>& /*temperatures*/) {
    static constexpr int cellSize = 2;
    static constexpr int sizeX = cellSize + 1;
    static constexpr int sizeY = 33;
    static constexpr int sizeZ = cellSize + 1;
    heightMap_ = generateHeightMap(heightMap_, chunkX * cellSize, 0, chunkZ * cellSize, sizeX, sizeY, sizeZ);
    for (int cellX = 0; cellX < cellSize; ++cellX) {
        for (int cellZ = 0; cellZ < cellSize; ++cellZ) {
            for (int cellY = 0; cellY < 32; ++cellY) {
                double density00 =
                    heightMap_[static_cast<std::size_t>(((cellX + 0) * sizeZ + (cellZ + 0)) * sizeY + (cellY + 0))];
                double density01 =
                    heightMap_[static_cast<std::size_t>(((cellX + 0) * sizeZ + (cellZ + 1)) * sizeY + (cellY + 0))];
                double density10 =
                    heightMap_[static_cast<std::size_t>(((cellX + 1) * sizeZ + (cellZ + 0)) * sizeY + (cellY + 0))];
                double density11 =
                    heightMap_[static_cast<std::size_t>(((cellX + 1) * sizeZ + (cellZ + 1)) * sizeY + (cellY + 0))];
                const double dy0 =
                    (heightMap_[static_cast<std::size_t>(((cellX + 0) * sizeZ + (cellZ + 0)) * sizeY + (cellY + 1))] -
                     density00) *
                    0.25;
                const double dy1 =
                    (heightMap_[static_cast<std::size_t>(((cellX + 0) * sizeZ + (cellZ + 1)) * sizeY + (cellY + 1))] -
                     density01) *
                    0.25;
                const double dy2 =
                    (heightMap_[static_cast<std::size_t>(((cellX + 1) * sizeZ + (cellZ + 0)) * sizeY + (cellY + 1))] -
                     density10) *
                    0.25;
                const double dy3 =
                    (heightMap_[static_cast<std::size_t>(((cellX + 1) * sizeZ + (cellZ + 1)) * sizeY + (cellY + 1))] -
                     density11) *
                    0.25;
                for (int subY = 0; subY < 4; ++subY) {
                    double x0 = density00;
                    double x1 = density01;
                    const double dx0 = (density10 - density00) * 0.125;
                    const double dx1 = (density11 - density01) * 0.125;
                    for (int subX = 0; subX < 8; ++subX) {
                        double density = x0;
                        const double dz = (x1 - x0) * 0.125;
                        for (int subZ = 0; subZ < 8; ++subZ) {
                            const int localX = cellX * 8 + subX;
                            const int localY = cellY * 4 + subY;
                            const int localZ = cellZ * 8 + subZ;
                            int blockId = 0;
                            if (density > 0.0) {
                                blockId = Block::STONE->id;
                            }
                            Generator::setRawBlock(chunk, localX, localY, localZ, blockId);
                            density += dz;
                        }
                        x0 += dx0;
                        x1 += dx1;
                    }
                    density00 += dy0;
                    density01 += dy1;
                    density10 += dy2;
                    density11 += dy3;
                }
            }
        }
    }
}

void SkyChunkGenerator::buildSurfaces(int chunkX, int chunkZ, Chunk& chunk, const std::vector<Biome*>& biomes) {
    constexpr double scale = 0.03125;
    sandBuffer_ = perlinNoise2_.create(sandBuffer_, chunkX * 16, chunkZ * 16, 0.0, 16, 16, 1, scale, scale, 1.0);
    gravelBuffer_ =
        perlinNoise2_.create(gravelBuffer_, chunkX * 16, 109.0134, chunkZ * 16, 16, 1, 16, scale, 1.0, scale);
    depthBuffer_ = perlinNoise3_.create(
        depthBuffer_, chunkX * 16, chunkZ * 16, 0.0, 16, 16, 1, scale * 2.0, scale * 2.0, scale * 2.0);
    for (int z = 0; z < 16; ++z) {
        for (int x = 0; x < 16; ++x) {
            const std::size_t surfaceIndex = static_cast<std::size_t>(z + x * 16);
            const Biome& biome = *biomes[surfaceIndex];
            int depth = static_cast<int>(depthBuffer_[surfaceIndex] / 3.0 + 3.0 + random_.nextDouble() * 0.25);
            int run = -1;
            std::uint8_t top = biome.topBlockId;
            std::uint8_t soil = biome.soilBlockId;
            for (int y = 127; y >= 0; --y) {
                const int blockId = Generator::rawBlock(chunk, x, y, z);
                if (blockId == 0) {
                    run = -1;
                    continue;
                }
                if (blockId != Block::STONE->id) {
                    continue;
                }
                if (run == -1) {
                    if (depth <= 0) {
                        top = 0;
                        soil = static_cast<std::uint8_t>(Block::STONE->id);
                    }
                    run = depth;
                    Generator::setRawBlock(chunk, x, y, z, y >= 0 ? top : soil);
                    continue;
                }
                if (run <= 0) {
                    continue;
                }
                Generator::setRawBlock(chunk, x, y, z, soil);
                --run;
                if (run == 0 && soil == static_cast<std::uint8_t>(Block::SAND->id)) {
                    run = random_.nextInt(4);
                    soil = static_cast<std::uint8_t>(Block::SANDSTONE->id);
                }
            }
        }
    }
}

std::vector<double>& SkyChunkGenerator::generateHeightMap(
    std::vector<double>& heightMap, int x, int y, int z, int sizeX, int sizeY, int sizeZ) {
    const std::size_t size = static_cast<std::size_t>(sizeX * sizeY * sizeZ);
    if (heightMap.size() < size) {
        heightMap.assign(size, 0.0);
    }
    double baseScale = 684.412;
    const double verticalScale = baseScale;
    baseScale *= 2.0;
    const BiomeSource* biomeSource = activeBiomeSource();
    const std::vector<double>& temperatureMap = biomeSource->temperatureMap();
    const std::vector<double>& downfallMap = biomeSource->downfallMap();
    scaleNoiseBuffer_ = floatingIslandScale_.create(scaleNoiseBuffer_, x, z, sizeX, sizeZ, 1.121, 1.121, 0.5);
    depthNoiseBuffer_ = floatingIslandNoise_.create(depthNoiseBuffer_, x, z, sizeX, sizeZ, 200.0, 200.0, 0.5);
    perlinNoiseBuffer_ = perlinNoise1_.create(
        perlinNoiseBuffer_, x, y, z, sizeX, sizeY, sizeZ, baseScale / 80.0, verticalScale / 160.0, baseScale / 80.0);
    minLimitPerlinNoiseBuffer_ = minLimitPerlinNoise_.create(
        minLimitPerlinNoiseBuffer_, x, y, z, sizeX, sizeY, sizeZ, baseScale, verticalScale, baseScale);
    maxLimitPerlinNoiseBuffer_ = maxLimitPerlinNoise_.create(
        maxLimitPerlinNoiseBuffer_, x, y, z, sizeX, sizeY, sizeZ, baseScale, verticalScale, baseScale);
    int index = 0;
    int horizontalIndex = 0;
    const int biomeStep = 16 / sizeX;
    for (int ix = 0; ix < sizeX; ++ix) {
        const int biomeX = ix * biomeStep + biomeStep / 2;
        for (int iz = 0; iz < sizeZ; ++iz) {
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
            if (depthNoise < 0.0) {
                depthNoise = -depthNoise * 0.3;
            }
            depthNoise = depthNoise * 3.0 - 2.0;
            if (depthNoise > 1.0) {
                depthNoise = 1.0;
            }
            depthNoise /= 8.0;
            depthNoise = 0.0;
            if (scaleNoise < 0.0) {
                scaleNoise = 0.0;
            }
            scaleNoise += 0.5;
            depthNoise = depthNoise * static_cast<double>(sizeY) / 16.0;
            ++horizontalIndex;
            const double center = static_cast<double>(sizeY) / 2.0;
            for (int iy = 0; iy < sizeY; ++iy) {
                double vertical = (static_cast<double>(iy) - center) * 8.0 / scaleNoise;
                if (vertical < 0.0) {
                    vertical *= -1.0;
                }
                const double minLimit = minLimitPerlinNoiseBuffer_[static_cast<std::size_t>(index)] / 512.0;
                const double maxLimit = maxLimitPerlinNoiseBuffer_[static_cast<std::size_t>(index)] / 512.0;
                const double selector = (perlinNoiseBuffer_[static_cast<std::size_t>(index)] / 10.0 + 1.0) / 2.0;
                double density = selector < 0.0
                                     ? minLimit
                                     : (selector > 1.0 ? maxLimit : minLimit + (maxLimit - minLimit) * selector);
                density -= 8.0;
                constexpr int topFadeHeight = 32;
                if (iy > sizeY - topFadeHeight) {
                    const double topFade =
                        static_cast<double>(iy - (sizeY - topFadeHeight)) / static_cast<double>(topFadeHeight - 1);
                    density = density * (1.0 - topFade) + -30.0 * topFade;
                }
                constexpr int bottomFadeHeight = 8;
                if (iy < bottomFadeHeight) {
                    const double bottomFade =
                        static_cast<double>(bottomFadeHeight - iy) / static_cast<double>(bottomFadeHeight - 1);
                    density = density * (1.0 - bottomFade) + -30.0 * bottomFade;
                }
                heightMap[static_cast<std::size_t>(index++)] = density;
            }
        }
    }
    return heightMap;
}
}  // namespace net::minecraft
