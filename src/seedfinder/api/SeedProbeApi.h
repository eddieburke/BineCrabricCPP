#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Wire values — no C++ enum in ABI */
#define SEEDFINDER_DIM_OVERWORLD 0u
#define SEEDFINDER_DIM_NETHER    255u
#define SEEDFINDER_DIM_SKYLANDS  1u

#define SEEDFINDER_DEPTH_BIOME_ONLY     0u
#define SEEDFINDER_DEPTH_TERRAIN        1u
#define SEEDFINDER_DEPTH_FULL_DECORATE  2u

#define SEEDFINDER_API_VERSION 1u

typedef struct SeedProbeRegion {
    int32_t origin_x;
    int32_t origin_z;
    int32_t radius_chunks;
    int32_t cave_margin_chunks;
} SeedProbeRegion;

typedef struct SeedProbeRequest {
    uint64_t seed;
    uint8_t dimension;
    SeedProbeRegion region;
    uint8_t depth;
    uint8_t compute_spawn;
    uint8_t include_biome_grid;
    uint8_t include_feature_hits;
    uint8_t include_block_histogram;
    uint64_t feature_flags;
    uint16_t api_version;
} SeedProbeRequest;

typedef struct SeedProbeSpawn {
    int32_t x;
    int32_t y;
    int32_t z;
    uint8_t surface_block_id;
    uint8_t biome_id;
    uint8_t valid;
    uint8_t on_sand_beach;
} SeedProbeSpawn;

typedef struct SeedProbeCounts {
    uint32_t coal_ore;
    uint32_t iron_ore;
    uint32_t gold_ore;
    uint32_t diamond_ore;
    uint32_t redstone_ore;
    uint32_t lapis_ore;
    uint32_t clay_blob;
    uint32_t dungeon;
    uint32_t water_lake;
    uint32_t lava_lake;
    uint32_t water_spring;
    uint32_t lava_spring;
    uint32_t trees;
    uint32_t pumpkin_patch;
    uint32_t sugar_cane;
    uint32_t brown_mushroom;
    uint32_t red_mushroom;
    uint32_t cactus;
    uint32_t snow_blocks;
    uint32_t glowstone;
    uint32_t nether_fire;
} SeedProbeCounts;

typedef struct SeedProbePoint {
    int32_t x;
    int32_t z;
    uint8_t biome_id;
    int32_t surface_y;
    float temperature;
    float downfall;
} SeedProbePoint;

typedef struct SeedProbeFeatureHit {
    uint8_t kind;
    int32_t x;
    int32_t y;
    int32_t z;
    uint8_t subtype;
    uint16_t extra;
} SeedProbeFeatureHit;

typedef struct SeedProbeResult {
    uint64_t seed;
    uint8_t dimension;
    uint8_t depth_reached;
    SeedProbeSpawn spawn;
    SeedProbeCounts counts;
    float avg_surface_y;
    float underwater_percent;
    uint8_t unique_biome_count;
    uint8_t dominant_biome_id;
    uint8_t dominant_biome_percent;
    uint16_t api_version;

    SeedProbePoint* biome_grid;
    uint32_t biome_grid_len;

    SeedProbeFeatureHit* feature_hits;
    uint32_t feature_hits_len;

    uint32_t block_histogram[256];

    uint32_t max_cactus_height;
    uint32_t desert_chunk_count;
} SeedProbeResult;

struct SeedProbeScratch;

int seedfinder_probe(
    const SeedProbeRequest* req,
    SeedProbeScratch* scratch,
    SeedProbeResult* out,
    char* error_buf,
    size_t error_len);

struct SeedProbeScratch* seedfinder_scratch_create(void);
void seedfinder_scratch_destroy(struct SeedProbeScratch* scratch);
void seedfinder_result_clear(SeedProbeResult* out);

#ifdef __cplusplus
}
#endif
