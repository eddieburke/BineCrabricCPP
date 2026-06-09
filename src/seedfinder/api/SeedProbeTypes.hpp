#pragma once

#include "SeedProbeApi.h"

#include <cstdint>

namespace seedfinder {

using SeedProbeDimension = std::uint8_t;
constexpr SeedProbeDimension kDimOverworld = SEEDFINDER_DIM_OVERWORLD;
constexpr SeedProbeDimension kDimNether = SEEDFINDER_DIM_NETHER;
constexpr SeedProbeDimension kDimSkylands = SEEDFINDER_DIM_SKYLANDS;

using SeedProbeDepth = std::uint8_t;
constexpr SeedProbeDepth kDepthBiomeOnly = SEEDFINDER_DEPTH_BIOME_ONLY;
constexpr SeedProbeDepth kDepthTerrain = SEEDFINDER_DEPTH_TERRAIN;
constexpr SeedProbeDepth kDepthFullDecorate = SEEDFINDER_DEPTH_FULL_DECORATE;

inline void initDefaultRequest(SeedProbeRequest& req)
{
    req.seed = 0;
    req.dimension = kDimOverworld;
    req.region.origin_x = 0;
    req.region.origin_z = 0;
    req.region.radius_chunks = 4;
    req.region.cave_margin_chunks = 8;
    req.depth = kDepthTerrain;
    req.compute_spawn = 0;
    req.include_biome_grid = 0;
    req.include_feature_hits = 0;
    req.include_block_histogram = 0;
    req.feature_flags = 0;
    req.api_version = SEEDFINDER_API_VERSION;
}

inline void initDefaultResult(SeedProbeResult& out)
{
    out.seed = 0;
    out.dimension = kDimOverworld;
    out.depth_reached = kDepthBiomeOnly;
    out.spawn = {};
    out.spawn.y = 64;
    out.counts = {};
    out.avg_surface_y = 0.f;
    out.underwater_percent = 0.f;
    out.unique_biome_count = 0;
    out.dominant_biome_id = 0;
    out.dominant_biome_percent = 0;
    out.api_version = SEEDFINDER_API_VERSION;
    out.biome_grid = nullptr;
    out.biome_grid_len = 0;
    out.feature_hits = nullptr;
    out.feature_hits_len = 0;
    for (std::uint32_t i = 0; i < 256; ++i) {
        out.block_histogram[i] = 0;
    }
    out.max_cactus_height = 0;
    out.desert_chunk_count = 0;
}

} // namespace seedfinder
