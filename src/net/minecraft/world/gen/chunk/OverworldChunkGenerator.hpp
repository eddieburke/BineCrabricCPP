#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/util/math/noise/OctavePerlinNoiseSampler.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/gen/carver/CaveWorldCarver.hpp"
#include "net/minecraft/world/gen/Generator.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace net::minecraft {

class ChunkSource;
class World;

class OverworldChunkGenerator {
public:
    OverworldChunkGenerator(World* world, std::uint64_t seed)
        : world_(world),
          seed_(seed),
          random_(seed),
          constructionRandom_(seed),
          minLimitPerlinNoise_(constructionRandom_, 16),
          maxLimitPerlinNoise_(constructionRandom_, 16),
          perlinNoise1_(constructionRandom_, 8),
          perlinNoise2_(constructionRandom_, 4),
          perlinNoise3_(constructionRandom_, 4),
          floatingIslandScale_(constructionRandom_, 10),
          floatingIslandNoise_(constructionRandom_, 16),
          forestNoise_(constructionRandom_, 8),
          biomeSource_(seed)
    {
    }

    void setWorld(World* world) noexcept
    {
        world_ = world;
    }

    [[nodiscard]] Chunk loadChunk(ChunkSource* source, int chunkX, int chunkZ);
    void decorate(ChunkSource* source, int chunkX, int chunkZ);

private:
    [[nodiscard]] BiomeSource* activeBiomeSource();
    [[nodiscard]] const BiomeSource* activeBiomeSource() const;
    std::vector<double>& generateHeightMap(std::vector<double>& heightMap, int x, int y, int z, int sizeX, int sizeY, int sizeZ);

    void buildTerrain(int chunkX, int chunkZ, Chunk& chunk, const std::vector<double>& temperatures)
    {
        static constexpr int cellSize = 4;
        static constexpr int seaLevel = 64;
        static constexpr int sizeX = cellSize + 1;
        static constexpr int sizeY = 17;
        static constexpr int sizeZ = cellSize + 1;
        heightMap_ = generateHeightMap(heightMap_, chunkX * cellSize, 0, chunkZ * cellSize, sizeX, sizeY, sizeZ);

        for (int cellX = 0; cellX < cellSize; ++cellX) {
            for (int cellZ = 0; cellZ < cellSize; ++cellZ) {
                for (int cellY = 0; cellY < 16; ++cellY) {
                    double density00 = heightMap_[static_cast<std::size_t>(((cellX + 0) * sizeZ + (cellZ + 0)) * sizeY + (cellY + 0))];
                    double density01 = heightMap_[static_cast<std::size_t>(((cellX + 0) * sizeZ + (cellZ + 1)) * sizeY + (cellY + 0))];
                    double density10 = heightMap_[static_cast<std::size_t>(((cellX + 1) * sizeZ + (cellZ + 0)) * sizeY + (cellY + 0))];
                    double density11 = heightMap_[static_cast<std::size_t>(((cellX + 1) * sizeZ + (cellZ + 1)) * sizeY + (cellY + 0))];
                    const double dy0 = (heightMap_[static_cast<std::size_t>(((cellX + 0) * sizeZ + (cellZ + 0)) * sizeY + (cellY + 1))] - density00) * 0.125;
                    const double dy1 = (heightMap_[static_cast<std::size_t>(((cellX + 0) * sizeZ + (cellZ + 1)) * sizeY + (cellY + 1))] - density01) * 0.125;
                    const double dy2 = (heightMap_[static_cast<std::size_t>(((cellX + 1) * sizeZ + (cellZ + 0)) * sizeY + (cellY + 1))] - density10) * 0.125;
                    const double dy3 = (heightMap_[static_cast<std::size_t>(((cellX + 1) * sizeZ + (cellZ + 1)) * sizeY + (cellY + 1))] - density11) * 0.125;

                    for (int subY = 0; subY < 8; ++subY) {
                        double x0 = density00;
                        double x1 = density01;
                        const double dx0 = (density10 - density00) * 0.25;
                        const double dx1 = (density11 - density01) * 0.25;
                        for (int subX = 0; subX < 4; ++subX) {
                            double density = x0;
                            const double dz = (x1 - x0) * 0.25;
                            for (int subZ = 0; subZ < 4; ++subZ) {
                                const int localX = cellX * 4 + subX;
                                const int localY = cellY * 8 + subY;
                                const int localZ = cellZ * 4 + subZ;
                                const double temperature = temperatures[static_cast<std::size_t>(localX * 16 + localZ)];
                                int blockId = 0;
                                if (localY < seaLevel) {
                                    blockId = temperature < 0.5 && localY >= seaLevel - 1 ? Block::ICE->id : Block::WATER->id;
                                }
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

    void buildSurfaces(int chunkX, int chunkZ, Chunk& chunk, const std::vector<BiomeInfo>& biomes)
    {
        static constexpr int seaLevel = 64;
        constexpr double scale = 0.03125;
        sandBuffer_ = perlinNoise2_.create(sandBuffer_, chunkX * 16, chunkZ * 16, 0.0, 16, 16, 1, scale, scale, 1.0);
        gravelBuffer_ = perlinNoise2_.create(gravelBuffer_, chunkX * 16, 109.0134, chunkZ * 16, 16, 1, 16, scale, 1.0, scale);
        depthBuffer_ = perlinNoise3_.create(depthBuffer_, chunkX * 16, chunkZ * 16, 0.0, 16, 16, 1, scale * 2.0, scale * 2.0, scale * 2.0);

        for (int x = 0; x < 16; ++x) {
            for (int z = 0; z < 16; ++z) {
                const BiomeInfo& biome = biomes[static_cast<std::size_t>(x + z * 16)];
                const bool sand = sandBuffer_[static_cast<std::size_t>(x + z * 16)] + random_.nextDouble() * 0.2 > 0.0;
                const bool gravel = gravelBuffer_[static_cast<std::size_t>(x + z * 16)] + random_.nextDouble() * 0.2 > 3.0;
                int depth = static_cast<int>(depthBuffer_[static_cast<std::size_t>(x + z * 16)] / 3.0 + 3.0 + random_.nextDouble() * 0.25);
                int run = -1;
                std::uint8_t top = biome.topBlockId;
                std::uint8_t soil = biome.soilBlockId;

                for (int y = 127; y >= 0; --y) {
                    if (y <= random_.nextInt(5)) {
                        Generator::setRawBlock(chunk, x, y, z, Block::BEDROCK->id);
                        continue;
                    }
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
                        } else if (y >= seaLevel - 4 && y <= seaLevel + 1) {
                            top = biome.topBlockId;
                            soil = biome.soilBlockId;
                            if (gravel) {
                                top = 0;
                                soil = static_cast<std::uint8_t>(Block::GRAVEL->id);
                            }
                            if (sand) {
                                top = static_cast<std::uint8_t>(Block::SAND->id);
                                soil = static_cast<std::uint8_t>(Block::SAND->id);
                            }
                        }
                        if (y < seaLevel && top == 0) {
                            top = static_cast<std::uint8_t>(Block::WATER->id);
                        }
                        run = depth;
                        Generator::setRawBlock(chunk, x, y, z, y >= seaLevel - 1 ? top : soil);
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

    World* world_ = nullptr;
    std::uint64_t seed_ = 0;
    JavaRandom random_;
    JavaRandom constructionRandom_;
    OctavePerlinNoiseSampler minLimitPerlinNoise_;
    OctavePerlinNoiseSampler maxLimitPerlinNoise_;
    OctavePerlinNoiseSampler perlinNoise1_;
    OctavePerlinNoiseSampler perlinNoise2_;
    OctavePerlinNoiseSampler perlinNoise3_;
    OctavePerlinNoiseSampler floatingIslandScale_;
    OctavePerlinNoiseSampler floatingIslandNoise_;
    OctavePerlinNoiseSampler forestNoise_;
    BiomeSource biomeSource_;
    CaveWorldCarver cave_;
    std::vector<BiomeInfo> biomes_;
    std::vector<double> heightMap_;
    std::vector<double> sandBuffer_;
    std::vector<double> gravelBuffer_;
    std::vector<double> depthBuffer_;
    std::vector<double> perlinNoiseBuffer_;
    std::vector<double> minLimitPerlinNoiseBuffer_;
    std::vector<double> maxLimitPerlinNoiseBuffer_;
    std::vector<double> scaleNoiseBuffer_;
    std::vector<double> depthNoiseBuffer_;
    std::vector<double> decorateTemperatures_;
};

} // namespace net::minecraft
