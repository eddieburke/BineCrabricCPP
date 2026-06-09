#include "seedfinder/metric/Metric.hpp"

#include "seedfinder/engine/Pass.hpp"

#include <cmath>
#include <cstdlib>
#include <limits>
#include <queue>
#include <vector>

namespace seedfinder::metric {

namespace {

namespace P = ::seedfinder::pass;

// Stable ids. Never renumber — they are serialized into saved graphs.
enum Id : int {
    SpawnBiome = 1,
    SpawnX = 2,
    SpawnZ = 3,
    SpawnDistance = 4,
    SpawnSurfaceBlock = 5,
    DominantBiome = 6,
    UniqueBiomeCount = 7,
    BiomeCoveragePct = 8,
    BiomeAt = 9,
    AvgTemperature = 10,
    AvgDownfall = 11,
    AvgSurfaceY = 12,
    MinSurfaceY = 13,
    MaxSurfaceY = 14,
    Flatness = 15,
    UnderwaterPct = 16,
    CaveAirVolume = 17,
    MaxCaveOpening = 18,
    AirBelowGravel = 19,
    ExposedCavePct = 20,
    CoalOre = 21,
    IronOre = 22,
    GoldOre = 23,
    DiamondOre = 24,
    RedstoneOre = 25,
    LapisOre = 26,
    Clay = 27,
    DiamondsExposedToCave = 28,
    Dungeons = 29,
    WaterLakes = 30,
    LavaLakes = 31,
    WaterSprings = 32,
    LavaSprings = 33,
    Trees = 34,
    CactusMaxHeight = 35,
    SugarCane = 36,
    Pumpkins = 37,
    BrownMushrooms = 38,
    RedMushrooms = 39,
    Flowers = 40,
    SnowBlocks = 41,
    BiomeContiguousRadius = 42,
    BiomeBlobCompactness = 43,
};

constexpr std::uint32_t kTerrain = P::Terrain | P::Surface;

const std::vector<MetricDef>& registry()
{
    static const std::vector<MetricDef> metrics = {
        {SpawnBiome, "spawn_biome", "Spawn biome", "Spawn", ValueKind::Biome, P::Biome, true, false, false},
        {SpawnX, "spawn_x", "Spawn X", "Spawn", ValueKind::Int, P::Biome, true, false, false},
        {SpawnZ, "spawn_z", "Spawn Z", "Spawn", ValueKind::Int, P::Biome, true, false, false},
        {SpawnDistance, "spawn_distance", "Spawn distance from 0,0", "Spawn", ValueKind::Float, P::Biome, true, false, false},
        {SpawnSurfaceBlock, "spawn_surface_block", "Spawn surface block", "Spawn", ValueKind::BlockId, kTerrain, true, false, false},
        {DominantBiome, "dominant_biome", "Dominant biome", "Biome", ValueKind::Biome, P::Biome, false, false, false},
        {UniqueBiomeCount, "unique_biome_count", "Unique biome count", "Biome", ValueKind::Int, P::Biome, false, false, false},
        {BiomeCoveragePct, "biome_coverage_pct", "Biome coverage %", "Biome", ValueKind::Percent, P::Biome, false, true, false},
        {BiomeAt, "biome_at", "Biome at X,Z", "Biome", ValueKind::Biome, P::Biome, false, false, true},
        {AvgTemperature, "avg_temperature", "Average temperature", "Biome", ValueKind::Float, P::Biome, false, false, false},
        {AvgDownfall, "avg_downfall", "Average downfall", "Biome", ValueKind::Float, P::Biome, false, false, false},
        {AvgSurfaceY, "avg_surface_y", "Average surface Y", "Terrain", ValueKind::Float, kTerrain, false, false, false},
        {MinSurfaceY, "min_surface_y", "Min surface Y", "Terrain", ValueKind::Int, kTerrain, false, false, false},
        {MaxSurfaceY, "max_surface_y", "Max surface Y", "Terrain", ValueKind::Int, kTerrain, false, false, false},
        {Flatness, "flatness", "Flatness (surface stddev)", "Terrain", ValueKind::Float, kTerrain, false, false, false},
        {UnderwaterPct, "underwater_pct", "Underwater %", "Terrain", ValueKind::Percent, kTerrain, false, false, false},
        {CaveAirVolume, "cave_air_volume", "Cave air volume", "Caves", ValueKind::Int, P::Caves, false, false, false},
        {MaxCaveOpening, "max_cave_opening", "Max cave opening", "Caves", ValueKind::Int, P::Caves, false, false, false},
        {AirBelowGravel, "air_below_gravel", "Air-below-gravel count", "Caves", ValueKind::Int, P::Caves, false, false, false},
        {ExposedCavePct, "exposed_cave_pct", "Exposed cave %", "Caves", ValueKind::Percent, P::Caves, false, false, false},
        {CoalOre, "coal_ore", "Coal ore", "Ores", ValueKind::Int, P::Ores, false, false, false},
        {IronOre, "iron_ore", "Iron ore", "Ores", ValueKind::Int, P::Ores, false, false, false},
        {GoldOre, "gold_ore", "Gold ore", "Ores", ValueKind::Int, P::Ores, false, false, false},
        {DiamondOre, "diamond_ore", "Diamond ore", "Ores", ValueKind::Int, P::Ores, false, false, false},
        {RedstoneOre, "redstone_ore", "Redstone ore", "Ores", ValueKind::Int, P::Ores, false, false, false},
        {LapisOre, "lapis_ore", "Lapis ore", "Ores", ValueKind::Int, P::Ores, false, false, false},
        {Clay, "clay", "Clay", "Ores", ValueKind::Int, P::Ores, false, false, false},
        {DiamondsExposedToCave, "diamonds_exposed_to_cave", "Diamonds exposed to cave", "Ores", ValueKind::Int, P::Ores | P::Caves, false, false, false},
        {Dungeons, "dungeons", "Dungeons", "Features", ValueKind::Int, P::Dungeons, false, false, false},
        {WaterLakes, "water_lakes", "Water lakes", "Features", ValueKind::Int, P::Lakes, false, false, false},
        {LavaLakes, "lava_lakes", "Lava lakes", "Features", ValueKind::Int, P::Lakes, false, false, false},
        {WaterSprings, "water_springs", "Water springs", "Features", ValueKind::Int, P::Springs, false, false, false},
        {LavaSprings, "lava_springs", "Lava springs", "Features", ValueKind::Int, P::Springs, false, false, false},
        {Trees, "trees", "Trees", "Features", ValueKind::Int, P::Trees, false, false, false},
        {CactusMaxHeight, "cactus_max_height", "Max cactus height", "Features", ValueKind::Int, P::Plants, false, false, false},
        {SugarCane, "sugar_cane", "Sugar cane", "Features", ValueKind::Int, P::Plants, false, false, false},
        {Pumpkins, "pumpkins", "Pumpkins", "Features", ValueKind::Int, P::Plants, false, false, false},
        {BrownMushrooms, "brown_mushrooms", "Brown mushrooms", "Features", ValueKind::Int, P::Plants, false, false, false},
        {RedMushrooms, "red_mushrooms", "Red mushrooms", "Features", ValueKind::Int, P::Plants, false, false, false},
        {Flowers, "flowers", "Flowers", "Features", ValueKind::Int, P::Plants, false, false, false},
        {SnowBlocks, "snow_blocks", "Snow blocks", "Features", ValueKind::Int, P::Snow, false, false, false},
        {BiomeContiguousRadius, "biome_contiguous_radius", "Biome contiguous radius", "Biome", ValueKind::Int, P::Biome, false, true, false},
        {BiomeBlobCompactness, "biome_blob_compactness", "Biome blob compactness %", "Biome", ValueKind::Percent, P::Biome, false, true, false},
    };
    return metrics;
}

struct BiomeAreaStats {
    int max_contiguous_radius = 0;
    double blob_compactness_percent = 0.0;
};

BiomeAreaStats biomeAreaStats(const ProbeResult& r, std::uint8_t biome)
{
    BiomeAreaStats stats;
    if (r.biome_grid.empty() || r.grid_side <= 0) {
        return stats;
    }

    const int side = r.grid_side;
    const int total = side * side;
    std::vector<std::uint8_t> match(static_cast<std::size_t>(total), 0);
    int matching = 0;
    for (int i = 0; i < total; ++i) {
        if (r.biome_grid[static_cast<std::size_t>(i)].biome_id == biome) {
            match[static_cast<std::size_t>(i)] = 1;
            ++matching;
        }
    }
    if (matching == 0) {
        return stats;
    }

    std::vector<std::uint8_t> visited(match.size(), 0);
    const int dx[4] = {1, -1, 0, 0};
    const int dz[4] = {0, 0, 1, -1};
    for (int start = 0; start < total; ++start) {
        if (match[static_cast<std::size_t>(start)] == 0 || visited[static_cast<std::size_t>(start)] != 0) {
            continue;
        }
        long long sumX = 0;
        long long sumZ = 0;
        int componentSize = 0;
        int componentRadius = 0;
        std::queue<int> open;
        open.push(start);
        visited[static_cast<std::size_t>(start)] = 1;
        while (!open.empty()) {
            const int idx = open.front();
            open.pop();
            const int lx = idx % side;
            const int lz = idx / side;
            sumX += lx;
            sumZ += lz;
            ++componentSize;

            for (int dir = 0; dir < 4; ++dir) {
                const int nx = lx + dx[dir];
                const int nz = lz + dz[dir];
                if (nx < 0 || nz < 0 || nx >= side || nz >= side) {
                    continue;
                }
                const int nidx = nz * side + nx;
                if (match[static_cast<std::size_t>(nidx)] == 0 || visited[static_cast<std::size_t>(nidx)] != 0) {
                    continue;
                }
                visited[static_cast<std::size_t>(nidx)] = 1;
                open.push(nidx);
            }
        }

        if (componentSize <= 0) {
            continue;
        }
        const int centerX = static_cast<int>(sumX / componentSize);
        const int centerZ = static_cast<int>(sumZ / componentSize);
        open.push(start);
        visited[static_cast<std::size_t>(start)] = 2;
        while (!open.empty()) {
            const int idx = open.front();
            open.pop();
            const int lx = idx % side;
            const int lz = idx / side;
            const int cheb = std::max(std::abs(lx - centerX), std::abs(lz - centerZ));
            if (cheb > componentRadius) {
                componentRadius = cheb;
            }
            for (int dir = 0; dir < 4; ++dir) {
                const int nx = lx + dx[dir];
                const int nz = lz + dz[dir];
                if (nx < 0 || nz < 0 || nx >= side || nz >= side) {
                    continue;
                }
                const int nidx = nz * side + nx;
                if (match[static_cast<std::size_t>(nidx)] == 0 || visited[static_cast<std::size_t>(nidx)] != 1) {
                    continue;
                }
                visited[static_cast<std::size_t>(nidx)] = 2;
                open.push(nidx);
            }
        }
        if (componentRadius > stats.max_contiguous_radius) {
            stats.max_contiguous_radius = componentRadius;
            stats.blob_compactness_percent =
                static_cast<double>(componentSize) * 100.0 / static_cast<double>(matching);
        }
    }

    return stats;
}

double biomeCoveragePct(const ProbeResult& r, std::uint8_t biome)
{
    if (r.biome_grid.empty()) {
        return 0.0;
    }
    std::size_t hits = 0;
    for (const BiomeCell& cell : r.biome_grid) {
        if (cell.biome_id == biome) {
            ++hits;
        }
    }
    return 100.0 * static_cast<double>(hits) / static_cast<double>(r.biome_grid.size());
}

double biomeAt(const ProbeResult& r, int x, int z)
{
    double best = std::numeric_limits<double>::max();
    std::uint8_t bestId = 0;
    for (const BiomeCell& cell : r.biome_grid) {
        const double dx = static_cast<double>(cell.x - x);
        const double dz = static_cast<double>(cell.z - z);
        const double d = dx * dx + dz * dz;
        if (d < best) {
            best = d;
            bestId = cell.biome_id;
        }
    }
    return static_cast<double>(bestId);
}

} // namespace

const std::vector<MetricDef>& allMetrics()
{
    return registry();
}

const MetricDef* metricById(int id)
{
    for (const MetricDef& m : registry()) {
        if (m.id == id) {
            return &m;
        }
    }
    return nullptr;
}

const MetricDef* metricByName(std::string_view name)
{
    for (const MetricDef& m : registry()) {
        if (name == m.name) {
            return &m;
        }
    }
    return nullptr;
}

bool metricNeedsSpawn(int id)
{
    const MetricDef* def = metricById(id);
    return def != nullptr && def->needsSpawn;
}

double evalMetric(int id, const MetricArgs& args, const ProbeResult& r)
{
    switch (id) {
    case SpawnBiome: return static_cast<double>(r.spawn_biome_id);
    case SpawnX: return static_cast<double>(r.spawn_x);
    case SpawnZ: return static_cast<double>(r.spawn_z);
    case SpawnDistance:
        return std::sqrt(static_cast<double>(r.spawn_x) * r.spawn_x + static_cast<double>(r.spawn_z) * r.spawn_z);
    case SpawnSurfaceBlock: return static_cast<double>(r.spawn_surface_block_id);
    case DominantBiome: return static_cast<double>(r.dominant_biome_id);
    case UniqueBiomeCount: return static_cast<double>(r.unique_biome_count);
    case BiomeCoveragePct: return biomeCoveragePct(r, args.biome);
    case BiomeAt: return biomeAt(r, args.x, args.z);
    case AvgTemperature: return r.avg_temperature;
    case AvgDownfall: return r.avg_downfall;
    case AvgSurfaceY: return r.avg_surface_y;
    case MinSurfaceY: return r.min_surface_y;
    case MaxSurfaceY: return r.max_surface_y;
    case Flatness: return r.flatness_stddev;
    case UnderwaterPct: return r.underwater_percent;
    case CaveAirVolume: return r.cave_air_volume;
    case MaxCaveOpening: return r.max_cave_opening;
    case AirBelowGravel: return r.air_below_gravel_count;
    case ExposedCavePct: return r.exposed_cave_percent;
    case CoalOre: return r.coal_ore;
    case IronOre: return r.iron_ore;
    case GoldOre: return r.gold_ore;
    case DiamondOre: return r.diamond_ore;
    case RedstoneOre: return r.redstone_ore;
    case LapisOre: return r.lapis_ore;
    case Clay: return r.clay;
    case DiamondsExposedToCave: return r.diamonds_exposed_to_cave;
    case Dungeons: return r.dungeons;
    case WaterLakes: return r.water_lakes;
    case LavaLakes: return r.lava_lakes;
    case WaterSprings: return r.water_springs;
    case LavaSprings: return r.lava_springs;
    case Trees: return r.trees;
    case CactusMaxHeight: return r.max_cactus_height;
    case SugarCane: return r.sugar_cane;
    case Pumpkins: return r.pumpkins;
    case BrownMushrooms: return r.brown_mushrooms;
    case RedMushrooms: return r.red_mushrooms;
    case Flowers: return r.flowers;
    case SnowBlocks: return r.snow_blocks;
    case BiomeContiguousRadius: return biomeAreaStats(r, args.biome).max_contiguous_radius;
    case BiomeBlobCompactness: return biomeAreaStats(r, args.biome).blob_compactness_percent;
    default: return 0.0;
    }
}

} // namespace seedfinder::metric
