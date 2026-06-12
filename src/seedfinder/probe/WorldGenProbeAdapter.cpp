#include "seedfinder/api/SeedProbeApi.h"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/Biomes.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/chunk/LegacyChunkCache.hpp"
#include "net/minecraft/world/storage/EmptyWorldStorage.hpp"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

namespace {

struct SeedProbeScratchImpl {
    std::unique_ptr<net::minecraft::EmptyWorldStorage> storage;
    std::unique_ptr<net::minecraft::World> world;
    std::unique_ptr<net::minecraft::BiomeSource> biome_source;
    std::uint64_t biome_source_seed = 0;
    bool biome_source_valid = false;
    std::vector<net::minecraft::BiomeInfo> biome_sample_buffer;
    std::vector<SeedProbePoint> biome_grid_buffer;
    std::vector<SeedProbeFeatureHit> feature_hit_buffer;
};

void writeError(char* error_buf, size_t error_len, const char* message)
{
    if (error_buf == nullptr || error_len == 0) {
        return;
    }
    std::strncpy(error_buf, message, error_len - 1);
    error_buf[error_len - 1] = '\0';
}

std::uint8_t biomeIdToWire(net::minecraft::BiomeId id)
{
    return static_cast<std::uint8_t>(id);
}

void ensureWorld(SeedProbeScratchImpl& scratch, std::int64_t seed)
{
    if (scratch.world == nullptr) {
        if (scratch.storage == nullptr) {
            scratch.storage = std::make_unique<net::minecraft::EmptyWorldStorage>();
        }
        scratch.world = std::make_unique<net::minecraft::World>(
            scratch.storage.get(), "seedfinder", seed, true);
        return;
    }
    scratch.world->resetForProbeSeed(seed);
}

net::minecraft::BiomeSource& ensureBiomeSource(SeedProbeScratchImpl& scratch, std::uint64_t seed)
{
    if (!scratch.biome_source_valid) {
        scratch.biome_source = std::make_unique<net::minecraft::BiomeSource>(seed);
        scratch.biome_source_seed = seed;
        scratch.biome_source_valid = true;
        return *scratch.biome_source;
    }
    if (scratch.biome_source_seed != seed) {
        scratch.biome_source->setSeed(seed);
        scratch.biome_source_seed = seed;
    }
    return *scratch.biome_source;
}

void prepareSpawn(net::minecraft::World& world, bool compute_spawn)
{
    if (!compute_spawn) {
        return;
    }
    world.setEventProcessingEnabled(true);
    world.initializeSpawnPoint();
}

void bindCacheSpawn(net::minecraft::World& world)
{
    auto* cache = dynamic_cast<net::minecraft::LegacyChunkCache*>(world.getChunkSource());
    if (cache == nullptr) {
        return;
    }
    const net::minecraft::Vec3i spawn = world.getSpawnPos();
    cache->setSpawnPoint(spawn.x >> 4, spawn.z >> 4);
}

void sampleBiomeOnly(
    net::minecraft::BiomeSource& source,
    const SeedProbeRequest& req,
    SeedProbeResult& out,
    SeedProbeScratchImpl& scratch)
{
    const int radius = req.region.radius_chunks;
    const int originChunkX = req.region.origin_x;
    const int originChunkZ = req.region.origin_z;

    std::vector<std::uint8_t> biomeHistogram(13, 0);
    std::uint32_t samples = 0;

    const int gridChunks = 2 * radius + 1;
    const int blockWidth = gridChunks * 16;
    const int blockStartX = (originChunkX - radius) * 16;
    const int blockStartZ = (originChunkZ - radius) * 16;

    source.getBiomesInArea(scratch.biome_sample_buffer, blockStartX, blockStartZ, blockWidth, blockWidth);
    const std::vector<double>& temperatureMap = source.temperatureMap();
    const std::vector<double>& downfallMap = source.downfallMap();

    if (req.include_biome_grid) {
        scratch.biome_grid_buffer.clear();
        scratch.biome_grid_buffer.reserve(static_cast<std::size_t>(gridChunks * gridChunks));
    }

    for (int iz = 0; iz < gridChunks; ++iz) {
        for (int ix = 0; ix < gridChunks; ++ix) {
            const int localX = ix * 16 + 8;
            const int localZ = iz * 16 + 8;
            const std::size_t sampleIndex = static_cast<std::size_t>(localX * blockWidth + localZ);
            const net::minecraft::BiomeInfo info = sampleIndex < scratch.biome_sample_buffer.size()
                ? scratch.biome_sample_buffer[sampleIndex]
                : net::minecraft::Biomes::getBiome(0.5, 0.5);
            const std::uint8_t biome_id = biomeIdToWire(info.id);
            if (static_cast<std::size_t>(biome_id) < biomeHistogram.size()) {
                ++biomeHistogram[biome_id];
            }
            ++samples;

            if (req.include_biome_grid) {
                const int wx = blockStartX + localX;
                const int wz = blockStartZ + localZ;
                SeedProbePoint point {};
                point.x = wx;
                point.z = wz;
                point.biome_id = biome_id;
                point.temperature = sampleIndex < temperatureMap.size()
                    ? static_cast<float>(temperatureMap[sampleIndex])
                    : 0.5f;
                point.downfall = sampleIndex < downfallMap.size()
                    ? static_cast<float>(downfallMap[sampleIndex])
                    : 0.5f;
                scratch.biome_grid_buffer.push_back(point);
            }
        }
    }

    std::uint8_t unique = 0;
    std::uint8_t dominant = 0;
    std::uint32_t dominantCount = 0;
    for (std::size_t i = 0; i < biomeHistogram.size(); ++i) {
        if (biomeHistogram[i] > 0) {
            ++unique;
            if (biomeHistogram[i] > dominantCount) {
                dominantCount = biomeHistogram[i];
                dominant = static_cast<std::uint8_t>(i);
            }
        }
    }

    out.unique_biome_count = unique;
    out.dominant_biome_id = dominant;
    out.dominant_biome_percent = samples > 0
        ? static_cast<std::uint8_t>((dominantCount * 100u) / samples)
        : 0;
    out.desert_chunk_count = biomeHistogram[static_cast<std::size_t>(biomeIdToWire(net::minecraft::BiomeId::Desert))];
    out.depth_reached = SEEDFINDER_DEPTH_BIOME_ONLY;

    if (req.include_biome_grid && !scratch.biome_grid_buffer.empty()) {
        out.biome_grid = static_cast<SeedProbePoint*>(
            std::malloc(scratch.biome_grid_buffer.size() * sizeof(SeedProbePoint)));
        if (out.biome_grid != nullptr) {
            std::memcpy(out.biome_grid, scratch.biome_grid_buffer.data(),
                scratch.biome_grid_buffer.size() * sizeof(SeedProbePoint));
            out.biome_grid_len = static_cast<std::uint32_t>(scratch.biome_grid_buffer.size());
        }
    }
}

void runTerrainTier(
    net::minecraft::World& world,
    const SeedProbeRequest& req,
    SeedProbeResult& out)
{
    auto* cache = dynamic_cast<net::minecraft::LegacyChunkCache*>(world.getChunkSource());
    if (cache == nullptr) {
        return;
    }

    const int radius = req.region.radius_chunks;
    const int margin = req.region.cave_margin_chunks;
    const int originChunkX = req.region.origin_x;
    const int originChunkZ = req.region.origin_z;

    for (int cz = originChunkZ - radius - margin; cz <= originChunkZ + radius + margin; ++cz) {
        for (int cx = originChunkX - radius - margin; cx <= originChunkX + radius + margin; ++cx) {
            cache->getChunk(cx, cz);
        }
    }

    float surfaceSum = 0.f;
    std::uint32_t surfaceSamples = 0;
    std::uint32_t underwater = 0;

    for (int cz = originChunkZ - radius; cz <= originChunkZ + radius; ++cz) {
        for (int cx = originChunkX - radius; cx <= originChunkX + radius; ++cx) {
            const int wx = cx * 16 + 8;
            const int wz = cz * 16 + 8;
            const int surfaceY = world.getTopSolidBlockY(wx, wz);
            if (surfaceY >= 0) {
                surfaceSum += static_cast<float>(surfaceY);
                ++surfaceSamples;
            }
            const int blockId = world.getSpawnBlockId(wx, wz);
            const bool isWater = net::minecraft::Block::WATER != nullptr
                && (blockId == net::minecraft::Block::WATER->id
                    || (net::minecraft::Block::FLOWING_WATER != nullptr
                        && blockId == net::minecraft::Block::FLOWING_WATER->id));
            if (isWater || surfaceY <= 62) {
                ++underwater;
            }
        }
    }

    out.avg_surface_y = surfaceSamples > 0 ? (surfaceSum / static_cast<float>(surfaceSamples)) : 0.f;
    const std::uint32_t totalCells = static_cast<std::uint32_t>((2 * radius + 1) * (2 * radius + 1));
    out.underwater_percent = totalCells > 0
        ? (static_cast<float>(underwater) * 100.f / static_cast<float>(totalCells))
        : 0.f;
    out.depth_reached = SEEDFINDER_DEPTH_TERRAIN;
}

void computeDecorateMetrics(
    net::minecraft::World& world,
    const SeedProbeRequest& req,
    SeedProbeResult& out);

void runFullDecorateTier(
    net::minecraft::World& world,
    const SeedProbeRequest& req,
    SeedProbeResult& out)
{
    auto* cache = dynamic_cast<net::minecraft::LegacyChunkCache*>(world.getChunkSource());
    if (cache == nullptr) {
        return;
    }

    const int radius = req.region.radius_chunks;
    const int originChunkX = req.region.origin_x;
    const int originChunkZ = req.region.origin_z;

    for (int cz = originChunkZ - radius; cz <= originChunkZ + radius; ++cz) {
        for (int cx = originChunkX - radius; cx <= originChunkX + radius; ++cx) {
            cache->getChunk(cx, cz);
            cache->getChunk(cx + 1, cz);
            cache->getChunk(cx, cz + 1);
            cache->getChunk(cx + 1, cz + 1);
            cache->decorate(cache, cx, cz);
        }
    }

    if (req.include_block_histogram) {
        for (int cz = originChunkZ - radius; cz <= originChunkZ + radius; ++cz) {
            for (int cx = originChunkX - radius; cx <= originChunkX + radius; ++cx) {
                net::minecraft::Chunk& chunk = cache->getChunk(cx, cz);
                for (std::size_t i = 0; i < chunk.blocks.size(); ++i) {
                    const std::uint8_t id = chunk.blocks[i];
                    out.block_histogram[id] += 1u;
                }
            }
        }
    }

    computeDecorateMetrics(world, req, out);

    out.depth_reached = SEEDFINDER_DEPTH_FULL_DECORATE;
}

void computeDecorateMetrics(
    net::minecraft::World& world,
    const SeedProbeRequest& req,
    SeedProbeResult& out)
{
    auto* cache = dynamic_cast<net::minecraft::LegacyChunkCache*>(world.getChunkSource());
    if (cache == nullptr) {
        return;
    }

    net::minecraft::BiomeSource* source = world.getBiomeSource();
    const int cactusId = net::minecraft::Block::CACTUS != nullptr
        ? net::minecraft::Block::CACTUS->id
        : 81;

    const int radius = req.region.radius_chunks;
    const int originChunkX = req.region.origin_x;
    const int originChunkZ = req.region.origin_z;

    std::uint32_t maxCactusHeight = 0;
    std::uint32_t desertChunks = 0;
    std::uint32_t cactusBlocks = 0;

    for (int cz = originChunkZ - radius; cz <= originChunkZ + radius; ++cz) {
        for (int cx = originChunkX - radius; cx <= originChunkX + radius; ++cx) {
            if (source != nullptr) {
                const int wx = cx * 16 + 8;
                const int wz = cz * 16 + 8;
                const net::minecraft::BiomeInfo info = source->getBiome(wx, wz);
                if (info.id == net::minecraft::BiomeId::Desert) {
                    ++desertChunks;
                }
            }

            net::minecraft::Chunk& chunk = cache->getChunk(cx, cz);
            for (int lz = 0; lz < 16; ++lz) {
                for (int lx = 0; lx < 16; ++lx) {
                    int columnRun = 0;
                    for (int y = 0; y < net::minecraft::Chunk::height; ++y) {
                        const int blockId = chunk.getBlockId(lx, y, lz);
                        if (blockId == cactusId) {
                            ++columnRun;
                            ++cactusBlocks;
                        } else if (columnRun > 0) {
                            break;
                        }
                    }
                    if (static_cast<std::uint32_t>(columnRun) > maxCactusHeight) {
                        maxCactusHeight = static_cast<std::uint32_t>(columnRun);
                    }
                }
            }
        }
    }

    out.max_cactus_height = maxCactusHeight;
    out.desert_chunk_count = desertChunks;
    out.counts.cactus = cactusBlocks;
}

void fillSpawn(net::minecraft::World& world, bool compute_spawn, SeedProbeResult& out)
{
    if (!compute_spawn) {
        out.spawn.valid = 0;
        return;
    }

    const net::minecraft::Vec3i spawn = world.getSpawnPos();
    out.spawn.x = spawn.x;
    out.spawn.y = spawn.y;
    out.spawn.z = spawn.z;
    out.spawn.surface_block_id = static_cast<std::uint8_t>(world.getSpawnBlockId(spawn.x, spawn.z));

    if (net::minecraft::BiomeSource* source = world.getBiomeSource()) {
        const net::minecraft::BiomeInfo info = source->getBiome(spawn.x, spawn.z);
        out.spawn.biome_id = biomeIdToWire(info.id);
    }

    out.spawn.on_sand_beach = net::minecraft::Block::SAND != nullptr
        && out.spawn.surface_block_id == static_cast<std::uint8_t>(net::minecraft::Block::SAND->id);
    out.spawn.valid = 1;
}

} // namespace

extern "C" {

struct SeedProbeScratch* seedfinder_scratch_create(void)
{
    return reinterpret_cast<SeedProbeScratch*>(new SeedProbeScratchImpl {});
}

void seedfinder_scratch_destroy(struct SeedProbeScratch* scratch)
{
    delete reinterpret_cast<SeedProbeScratchImpl*>(scratch);
}

void seedfinder_result_clear(SeedProbeResult* out)
{
    if (out == nullptr) {
        return;
    }
    if (out->biome_grid != nullptr) {
        std::free(out->biome_grid);
        out->biome_grid = nullptr;
    }
    if (out->feature_hits != nullptr) {
        std::free(out->feature_hits);
        out->feature_hits = nullptr;
    }
    out->biome_grid_len = 0;
    out->feature_hits_len = 0;
}

int seedfinder_probe(
    const SeedProbeRequest* req,
    SeedProbeScratch* scratch,
    SeedProbeResult* out,
    char* error_buf,
    size_t error_len)
{
    if (req == nullptr || scratch == nullptr || out == nullptr) {
        writeError(error_buf, error_len, "null argument");
        return 1;
    }
    if (req->dimension != SEEDFINDER_DIM_OVERWORLD) {
        writeError(error_buf, error_len, "v1 supports overworld only");
        return 2;
    }
    if (req->depth > SEEDFINDER_DEPTH_TERRAIN && req->depth != SEEDFINDER_DEPTH_FULL_DECORATE) {
        writeError(error_buf, error_len, "probe depth not supported in v1");
        return 3;
    }

    auto* impl = reinterpret_cast<SeedProbeScratchImpl*>(scratch);
    seedfinder_result_clear(out);
    out->seed = req->seed;
    out->dimension = req->dimension;
    out->depth_reached = SEEDFINDER_DEPTH_BIOME_ONLY;
    out->api_version = SEEDFINDER_API_VERSION;

    const std::uint64_t seed = req->seed;
    const bool compute_spawn = req->compute_spawn != 0;
    const bool biome_only = req->depth == SEEDFINDER_DEPTH_BIOME_ONLY;
    const bool fast_biome_only = biome_only && !compute_spawn;

    if (fast_biome_only) {
        net::minecraft::BiomeSource& source = ensureBiomeSource(*impl, seed);
        sampleBiomeOnly(source, *req, *out, *impl);
        return 0;
    }

    ensureWorld(*impl, static_cast<std::int64_t>(seed));
    net::minecraft::World& world = *impl->world;
    prepareSpawn(world, compute_spawn);
    bindCacheSpawn(world);
    fillSpawn(world, compute_spawn, *out);

    if (net::minecraft::BiomeSource* source = world.getBiomeSource()) {
        sampleBiomeOnly(*source, *req, *out, *impl);
    }

    if (req->depth >= SEEDFINDER_DEPTH_TERRAIN) {
        runTerrainTier(world, *req, *out);
    }

    if (req->depth >= SEEDFINDER_DEPTH_FULL_DECORATE) {
        runFullDecorateTier(world, *req, *out);
    }

    return 0;
}

} // extern "C"
