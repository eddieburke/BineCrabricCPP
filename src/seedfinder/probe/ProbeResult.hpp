#pragma once

#include <cstdint>
#include <vector>

// Plain C++ result of probing one seed. Deliberately free of net::minecraft types so
// the engine/graph/metric layers stay decoupled from the game (the probe extracts
// everything into primitives). Fields are filled tier-by-tier; a field is only
// meaningful if its producing pass is set in passesRun.

namespace seedfinder {

struct BiomeCell {
    int x = 0;          // world block X of the sampled chunk centre
    int z = 0;          // world block Z
    std::uint8_t biome_id = 0;
    int surface_y = -1; // top solid Y at the centre, or -1 if terrain not run
    float temperature = 0.5f;
    float downfall = 0.5f;
};

struct ProbeResult {
    std::uint64_t seed = 0;
    std::uint32_t passesRun = 0;

    // --- Spawn (needSpawn) ---
    bool spawn_valid = false;
    int spawn_x = 0;
    int spawn_y = 64;
    int spawn_z = 0;
    std::uint8_t spawn_biome_id = 0;
    std::uint8_t spawn_surface_block_id = 0;
    bool spawn_on_sand = false;

    // --- Biome grid (pass::Biome) ---
    std::vector<BiomeCell> biome_grid;
    int grid_side = 0;                 // biome_grid is grid_side * grid_side, row-major
    std::uint8_t dominant_biome_id = 0;
    std::uint8_t unique_biome_count = 0;
    std::uint8_t dominant_biome_percent = 0;
    float avg_temperature = 0.5f;
    float avg_downfall = 0.5f;

    // --- Terrain / surface (pass::Terrain / pass::Surface) ---
    float avg_surface_y = 0.f;
    int min_surface_y = 0;
    int max_surface_y = 0;
    float flatness_stddev = 0.f;       // stddev of per-column surface Y across the region
    float underwater_percent = 0.f;

    // --- Caves (pass::Caves) ---
    std::uint32_t cave_air_volume = 0;     // carved air cells below the surface column
    std::uint32_t max_cave_opening = 0;    // largest single-column carved run
    std::uint32_t air_below_gravel_count = 0; // gravel cells with air directly beneath
    float exposed_cave_percent = 0.f;      // % of columns whose surface sits over a void

    // --- Decorate counts, region totals (populate passes) ---
    std::uint32_t coal_ore = 0;
    std::uint32_t iron_ore = 0;
    std::uint32_t gold_ore = 0;
    std::uint32_t diamond_ore = 0;
    std::uint32_t redstone_ore = 0;
    std::uint32_t lapis_ore = 0;
    std::uint32_t clay = 0;
    std::uint32_t diamonds_exposed_to_cave = 0;
    std::uint32_t dungeons = 0;
    std::uint32_t water_lakes = 0;
    std::uint32_t lava_lakes = 0;
    std::uint32_t water_springs = 0;
    std::uint32_t lava_springs = 0;
    std::uint32_t trees = 0;
    std::uint32_t cactus = 0;
    std::uint32_t max_cactus_height = 0;
    std::uint32_t sugar_cane = 0;
    std::uint32_t pumpkins = 0;
    std::uint32_t brown_mushrooms = 0;
    std::uint32_t red_mushrooms = 0;
    std::uint32_t flowers = 0;
    std::uint32_t snow_blocks = 0;

    // Resets scalars to defaults and clears the grid while keeping its capacity, so a
    // per-thread result can be reused across seeds without reallocating.
    void reset()
    {
        std::vector<BiomeCell> recycled = std::move(biome_grid);
        recycled.clear();
        *this = ProbeResult{};
        biome_grid = std::move(recycled);
    }
};

} // namespace seedfinder
