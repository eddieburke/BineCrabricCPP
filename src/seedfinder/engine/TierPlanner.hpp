#pragma once

#include "seedfinder/api/SeedProbeTypes.hpp"
#include "seedfinder/config/ConfigSchema.hpp"

namespace seedfinder::engine {

struct TierPlan {
    SeedProbeDepth depth = kDepthBiomeOnly;
    bool need_spawn = false;
    bool need_biome_grid = false;
    bool need_feature_hits = false;
    bool need_block_histogram = false;
};

[[nodiscard]] TierPlan planTier(const config::SearchConfig& cfg);

[[nodiscard]] SeedProbeRequest buildProbeRequest(
    std::uint64_t seed,
    const config::SearchConfig& cfg,
    const TierPlan& plan);

} // namespace seedfinder::engine
