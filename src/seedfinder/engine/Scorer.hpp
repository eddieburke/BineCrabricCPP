#pragma once

#include "seedfinder/api/SeedProbeTypes.hpp"
#include "seedfinder/config/ConfigSchema.hpp"
#include "seedfinder/engine/BiomeCatalog.hpp"

#include <string>
#include <vector>

namespace seedfinder::engine {

struct ScoreResult {
    bool all_hard_constraints_met = false;
    float partial_match_score = 0.f;
    SeedProbeDepth search_tier_passed = kDepthBiomeOnly;
    std::vector<std::string> notes;
    std::vector<std::string> not_evaluated;
};

[[nodiscard]] ScoreResult scoreResult(
    const config::SearchConfig& cfg,
    const SeedProbeResult& result);

} // namespace seedfinder::engine
