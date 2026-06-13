#include "seedfinder/probe/ScanProbe.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/biome/Biome.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace seedfinder {
namespace {

using net::minecraft::Biome;
using net::minecraft::Block;
using net::minecraft::Chunk;

constexpr int kSeaLevel = 64;
constexpr int kHeight = Chunk::height;

[[nodiscard]] bool isSolidOrFluid(int blockId)
{
    if (blockId <= 0 || blockId >= static_cast<int>(Block::BLOCK_COUNT)) {
        return false;
    }
    const Block* block = Block::BLOCKS[static_cast<std::size_t>(blockId)];
    if (block == nullptr) {
        return false;
    }
    const auto& material = block->material;
    return material.blocksMovement() || material.isFluid();
}

// Faithful to World::getTopSolidBlockY on a raw chunk column: highest solid-or-fluid
// block + 1, or -1.
[[nodiscard]] int topSolidBlockY(const Chunk& chunk, int localX, int localZ)
{
    for (int y = kHeight - 1; y > 0; --y) {
        if (isSolidOrFluid(chunk.getBlockId(localX, y, localZ))) {
            return y + 1;
        }
    }
    return -1;
}

// Faithful to World::getSpawnBlockId on a raw chunk column: top non-air at/above y=63.
[[nodiscard]] int spawnBlockId(const Chunk& chunk, int localX, int localZ)
{
    int y = 63;
    while (y < kHeight - 1 && chunk.getBlockId(localX, y + 1, localZ) != 0) {
        ++y;
    }
    return chunk.getBlockId(localX, y, localZ);
}

[[nodiscard]] bool isWaterId(int blockId)
{
    if (Block::WATER != nullptr && blockId == Block::WATER->id) {
        return true;
    }
    return Block::FLOWING_WATER != nullptr && blockId == Block::FLOWING_WATER->id;
}

} // namespace

net::minecraft::OverworldChunkGenerator& ScanProbe::ensureGenerator(std::uint64_t seed)
{
    if (!generatorValid_ || generatorSeed_ != seed) {
        generator_.emplace(nullptr, seed);
        generatorSeed_ = seed;
        generatorValid_ = true;
    }
    return *generator_;
}

void ScanProbe::probe(std::uint64_t seed, int originChunkX, int originChunkZ, const PassPlan& plan, ProbeResult& out)
{
    out.seed = seed;
    out.passesRun |= plan.passes;

    if (plan.has(pass::Biome)) {
        sampleBiomeGrid(seed, originChunkX, originChunkZ, plan, out);
    }
    if (plan.has(pass::Terrain)) {
        runTerrainSide(seed, originChunkX, originChunkZ, plan, out);
    }
}

void ScanProbe::sampleBiomeGrid(
    std::uint64_t seed, int originChunkX, int originChunkZ, const PassPlan& plan, ProbeResult& out)
{
    const int radius = plan.radiusChunks;
    const int side = 2 * radius + 1;
    const int blockWidth = side * 16;
    const int blockStartX = (originChunkX - radius) * 16;
    const int blockStartZ = (originChunkZ - radius) * 16;

    biomeSource_.setSeed(seed);
    biomeSource_.getBiomesInArea(biomeArea_, blockStartX, blockStartZ, blockWidth, blockWidth);
    const std::vector<double>& temperatureMap = biomeSource_.temperatureMap();
    const std::vector<double>& downfallMap = biomeSource_.downfallMap();

    out.grid_side = side;
    out.biome_grid.reserve(static_cast<std::size_t>(side * side));

    std::uint32_t histogram[256] = {0};
    double tempSum = 0.0;
    double downfallSum = 0.0;
    std::uint32_t samples = 0;

    for (int iz = 0; iz < side; ++iz) {
        for (int ix = 0; ix < side; ++ix) {
            const int localX = ix * 16 + 8;
            const int localZ = iz * 16 + 8;
            const std::size_t sampleIndex = static_cast<std::size_t>(localX * blockWidth + localZ);
            Biome* biome = sampleIndex < biomeArea_.size()
                ? biomeArea_[sampleIndex]
                : &net::minecraft::Biome::getBiome(0.5, 0.5);
            if (biome == nullptr) {
                biome = &net::minecraft::Biome::getBiome(0.5, 0.5);
            }

            BiomeCell cell;
            cell.x = blockStartX + localX;
            cell.z = blockStartZ + localZ;
            cell.biome_id = static_cast<std::uint8_t>(biome->id);
            cell.surface_y = -1;
            cell.temperature = sampleIndex < temperatureMap.size() ? static_cast<float>(temperatureMap[sampleIndex]) : 0.5f;
            cell.downfall = sampleIndex < downfallMap.size() ? static_cast<float>(downfallMap[sampleIndex]) : 0.5f;
            out.biome_grid.push_back(cell);

            ++histogram[cell.biome_id];
            tempSum += cell.temperature;
            downfallSum += cell.downfall;
            ++samples;
        }
    }

    std::uint8_t unique = 0;
    std::uint8_t dominant = 0;
    std::uint32_t dominantCount = 0;
    for (int i = 0; i < 256; ++i) {
        if (histogram[i] > 0) {
            ++unique;
            if (histogram[i] > dominantCount) {
                dominantCount = histogram[i];
                dominant = static_cast<std::uint8_t>(i);
            }
        }
    }

    out.unique_biome_count = unique;
    out.dominant_biome_id = dominant;
    out.dominant_biome_percent = samples > 0 ? static_cast<std::uint8_t>((dominantCount * 100u) / samples) : 0;
    out.avg_temperature = samples > 0 ? static_cast<float>(tempSum / samples) : 0.5f;
    out.avg_downfall = samples > 0 ? static_cast<float>(downfallSum / samples) : 0.5f;
}

void ScanProbe::runTerrainSide(
    std::uint64_t seed, int originChunkX, int originChunkZ, const PassPlan& plan, ProbeResult& out)
{
    const int radius = plan.radiusChunks;
    const int side = 2 * radius + 1;
    // Caves require cross-chunk carving through a ChunkSource (a World), so they are
    // measured in the decorate tier; loadChunk(nullptr) yields uncarved terrain+surface.
    const bool doCaves = false;
    const int gravelId = Block::GRAVEL != nullptr ? Block::GRAVEL->id : -1;
    (void)gravelId;

    net::minecraft::OverworldChunkGenerator& generator = ensureGenerator(seed);

    double surfaceSum = 0.0;
    std::uint32_t surfaceSamples = 0;
    int minSurface = kHeight;
    int maxSurface = 0;
    std::uint32_t underwater = 0;
    std::vector<int> surfaceYs;
    surfaceYs.reserve(static_cast<std::size_t>(side * side));

    std::uint32_t caveAir = 0;
    std::uint32_t maxOpening = 0;
    std::uint32_t airBelowGravel = 0;
    std::uint32_t exposedColumns = 0;
    std::uint32_t totalColumns = 0;

    for (int iz = 0; iz < side; ++iz) {
        for (int ix = 0; ix < side; ++ix) {
            const int chunkX = (originChunkX - radius) + ix;
            const int chunkZ = (originChunkZ - radius) + iz;

            const Chunk chunk = generator.loadChunk(nullptr, chunkX, chunkZ);

            const int surfaceY = topSolidBlockY(chunk, 8, 8);
            if (surfaceY >= 0) {
                surfaceSum += surfaceY;
                ++surfaceSamples;
                minSurface = std::min(minSurface, surfaceY);
                maxSurface = std::max(maxSurface, surfaceY);
                surfaceYs.push_back(surfaceY);
            }
            if (isWaterId(spawnBlockId(chunk, 8, 8)) || surfaceY <= 62) {
                ++underwater;
            }

            const std::size_t cellIndex = static_cast<std::size_t>(iz * side + ix);
            if (cellIndex < out.biome_grid.size()) {
                out.biome_grid[cellIndex].surface_y = surfaceY;
            }

            if (doCaves) {
                for (int lz = 0; lz < 16; ++lz) {
                    for (int lx = 0; lx < 16; ++lx) {
                        const int colTop = topSolidBlockY(chunk, lx, lz);
                        const int top = colTop > 0 ? colTop : kSeaLevel;
                        ++totalColumns;
                        int run = 0;
                        bool columnHadAir = false;
                        for (int y = 1; y < top; ++y) {
                            const int id = chunk.getBlockId(lx, y, lz);
                            if (id == 0) {
                                ++caveAir;
                                ++run;
                                columnHadAir = true;
                                maxOpening = std::max(maxOpening, static_cast<std::uint32_t>(run));
                            } else {
                                run = 0;
                            }
                            if (id == gravelId && gravelId >= 0 && y > 0
                                && chunk.getBlockId(lx, y - 1, lz) == 0) {
                                ++airBelowGravel;
                            }
                        }
                        if (columnHadAir) {
                            ++exposedColumns;
                        }
                    }
                }
            }
        }
    }

    out.avg_surface_y = surfaceSamples > 0 ? static_cast<float>(surfaceSum / surfaceSamples) : 0.f;
    out.min_surface_y = surfaceSamples > 0 ? minSurface : 0;
    out.max_surface_y = maxSurface;
    const std::uint32_t totalCells = static_cast<std::uint32_t>(side * side);
    out.underwater_percent = totalCells > 0 ? (static_cast<float>(underwater) * 100.f / totalCells) : 0.f;

    if (!surfaceYs.empty()) {
        const double mean = surfaceSum / surfaceYs.size();
        double variance = 0.0;
        for (int y : surfaceYs) {
            const double d = y - mean;
            variance += d * d;
        }
        out.flatness_stddev = static_cast<float>(std::sqrt(variance / surfaceYs.size()));
    }

    if (doCaves) {
        out.cave_air_volume = caveAir;
        out.max_cave_opening = maxOpening;
        out.air_below_gravel_count = airBelowGravel;
        out.exposed_cave_percent = totalColumns > 0
            ? (static_cast<float>(exposedColumns) * 100.f / totalColumns)
            : 0.f;
    }
}

} // namespace seedfinder
