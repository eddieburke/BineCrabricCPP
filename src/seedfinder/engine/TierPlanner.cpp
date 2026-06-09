#include "seedfinder/engine/TierPlanner.hpp"

#include "seedfinder/engine/BiomeCatalog.hpp"

namespace seedfinder::engine {

namespace {

bool rulesNeedFullDecorate(const config::SearchConfig& cfg)
{
    for (const config::RuleNode& node : cfg.advanced_rule_nodes) {
        if (node.kind == config::RuleNodeKind::BlockConstraint
            || node.kind == config::RuleNodeKind::BlockOnBlock) {
            return true;
        }
        if (node.kind == config::RuleNodeKind::MetricObjective
            && node.metric == "max_cactus_height") {
            return true;
        }
    }
    return false;
}

bool rulesNeedBiomeGrid(const config::SearchConfig& cfg)
{
    if (!cfg.biome_at_required.empty()
        || cfg.unique_biome_min >= 0
        || cfg.unique_biome_max >= 0
        || cfg.required_biome_min_coverage_percent >= 0.f
        || cfg.forbidden_biome_max_coverage_percent >= 0.f
        || cfg.required_biome_min_contiguous_radius_chunks >= 1
        || cfg.required_biome_min_compactness_percent >= 0.f
        || cfg.temperature_min >= 0.f
        || cfg.temperature_max >= 0.f
        || cfg.downfall_min >= 0.f
        || cfg.downfall_max >= 0.f
        || cfg.require_snow
        || cfg.require_rain
        || !cfg.required_biomes.empty()) {
        return true;
    }
    for (const config::RuleNode& node : cfg.advanced_rule_nodes) {
        if (node.kind == config::RuleNodeKind::BiomeConstraint
            && (node.min_value >= 0.0 || node.max_value >= 0.0 || node.value >= 1.0)) {
            return true;
        }
        if (node.kind == config::RuleNodeKind::MetricObjective && metricNeedsBiomeGrid(node.metric)) {
            return true;
        }
    }
    return false;
}

} // namespace

TierPlan planTier(const config::SearchConfig& cfg)
{
    TierPlan plan;
    plan.depth = kDepthBiomeOnly;
    plan.need_spawn = cfg.compute_spawn
        || !cfg.spawn_biome.empty()
        || !cfg.spawn_biome_in.empty()
        || cfg.spawn_x >= 0
        || cfg.spawn_z >= 0
        || !cfg.spawn_surface_block.empty()
        || cfg.spawn_y_min >= 0
        || cfg.spawn_distance_chunks_max >= 0
        || cfg.spawn_chunk_x >= 0;

    const bool needsTerrain = cfg.avg_surface_y_min >= 0.f
        || cfg.avg_surface_y_max >= 0.f
        || cfg.underwater_percent_max >= 0.f
        || cfg.flatness_max >= 0.f
        || cfg.cave_proxy_min >= 0;

    if (needsTerrain || plan.need_spawn) {
        plan.depth = kDepthTerrain;
    }
    if (cfg.probe_depth_max < plan.depth) {
        plan.depth = cfg.probe_depth_max;
    }
    if (rulesNeedBiomeGrid(cfg)) {
        plan.need_biome_grid = true;
    }
    const bool needsDecorateMetrics = rulesNeedFullDecorate(cfg);
    if (needsDecorateMetrics && cfg.probe_depth_max >= kDepthFullDecorate) {
        plan.need_feature_hits = true;
        plan.need_block_histogram = true;
        plan.depth = kDepthFullDecorate;
    }
    return plan;
}

SeedProbeRequest buildProbeRequest(
    std::uint64_t seed,
    const config::SearchConfig& cfg,
    const TierPlan& plan)
{
    SeedProbeRequest req {};
    seedfinder::initDefaultRequest(req);
    req.seed = seed;
    req.dimension = kDimOverworld;
    req.region.origin_x = cfg.scan_origin_x;
    req.region.origin_z = cfg.scan_origin_z;
    req.region.radius_chunks = cfg.scan_radius_chunks;
    req.region.cave_margin_chunks = 8;
    req.depth = plan.depth;
    req.compute_spawn = plan.need_spawn ? 1 : 0;
    req.include_biome_grid = plan.need_biome_grid ? 1 : 0;
    req.include_feature_hits = plan.need_feature_hits ? 1 : 0;
    req.include_block_histogram = plan.need_block_histogram ? 1 : 0;
    return req;
}

} // namespace seedfinder::engine
