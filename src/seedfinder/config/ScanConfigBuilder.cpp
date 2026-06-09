#include "seedfinder/config/ScanConfigBuilder.hpp"

#include "seedfinder/api/SeedProbeTypes.hpp"
#include "seedfinder/engine/BiomeCatalog.hpp"
#include "seedfinder/graph/GraphBuilder.hpp"
#include "seedfinder/metric/Metric.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>

namespace seedfinder::config {
namespace {

using seedfinder::engine::biomeIdFromName;
using seedfinder::graph::CompareOp;
using seedfinder::graph::GraphBuilder;
using seedfinder::graph::ObjectiveDir;
using seedfinder::metric::MetricDef;

constexpr int kSpawnBiome = 1;
constexpr int kSpawnX = 2;
constexpr int kSpawnZ = 3;
constexpr int kSpawnSurfaceBlock = 5;
constexpr int kDominantBiome = 6;
constexpr int kUniqueBiomeCount = 7;
constexpr int kBiomeCoveragePct = 8;
constexpr int kBiomeAt = 9;
constexpr int kAvgTemperature = 10;
constexpr int kAvgDownfall = 11;
constexpr int kAvgSurfaceY = 12;
constexpr int kFlatness = 15;
constexpr int kUnderwaterPct = 16;
constexpr int kBiomeContiguousRadius = 42;
constexpr int kBiomeBlobCompactness = 43;

bool metricAllowedAtDepth(const MetricDef& def, SeedProbeDepth depth)
{
    if (depth >= kDepthFullDecorate) {
        return true;
    }
    if (depth >= kDepthTerrain) {
        return (def.requiredPasses & seedfinder::pass::Populate) == 0;
    }
    return (def.requiredPasses & ~seedfinder::pass::Biome) == 0;
}

bool canUseMetric(int metricId, SeedProbeDepth depth)
{
    const MetricDef* def = seedfinder::metric::metricById(metricId);
    return def != nullptr && metricAllowedAtDepth(*def, depth);
}

void addHardIf(GraphBuilder& builder, bool condition, int metricId, CompareOp op, double threshold,
    std::uint8_t biomeArg = 0, int argX = 0, int argZ = 0, SeedProbeDepth depth = kDepthFullDecorate)
{
    if (!condition || !canUseMetric(metricId, depth)) {
        return;
    }
    builder.addHardCompare(metricId, op, threshold, biomeArg, argX, argZ);
}

void addHardOrBiomes(GraphBuilder& builder, int metricId, CompareOp op, const std::vector<std::string>& biomes,
    SeedProbeDepth depth)
{
    if (biomes.empty() || !canUseMetric(metricId, depth)) {
        return;
    }
    std::vector<double> thresholds;
    thresholds.reserve(biomes.size());
    for (const std::string& biome : biomes) {
        thresholds.push_back(static_cast<double>(biomeIdFromName(biome)));
    }
    builder.addHardOrCompare(metricId, op, thresholds);
}

const MetricDef* resolveMetricName(std::string_view name)
{
    if (const MetricDef* def = seedfinder::metric::metricByName(name)) {
        return def;
    }
    if (name == "biome_coverage_percent") {
        return seedfinder::metric::metricByName("biome_coverage_pct");
    }
    if (name == "max_contiguous_biome_radius") {
        return seedfinder::metric::metricByName("biome_contiguous_radius");
    }
    if (name == "biome_blob_compactness_percent") {
        return seedfinder::metric::metricByName("biome_blob_compactness");
    }
    if (name == "max_cactus_height") {
        return seedfinder::metric::metricByName("cactus_max_height");
    }
    return nullptr;
}

std::uint8_t primaryBiomeArg(const std::vector<std::string>& names)
{
    if (names.empty()) {
        return 0;
    }
    return biomeIdFromName(names.front());
}

int blockIdFromName(std::string_view name)
{
    if (name == "sand") {
        return 12;
    }
    if (name == "grass") {
        return 2;
    }
    if (name == "gravel") {
        return 13;
    }
    return -1;
}

void applyRuleNode(GraphBuilder& builder, const RuleNode& node, SeedProbeDepth depth)
{
    if (node.kind == RuleNodeKind::BiomeConstraint) {
        const bool areaMode = node.min_value >= 0.0 || node.max_value >= 0.0 || node.value >= 1.0;
        const std::string mode = node.op.empty() ? "any_of" : node.op;
        const bool noneOf = mode == "none_of";
        if (areaMode) {
            const std::uint8_t biomeArg = primaryBiomeArg(node.values);
            if (node.min_value >= 0.0) {
                addHardIf(builder, true, kBiomeCoveragePct, CompareOp::Ge, node.min_value, biomeArg, 0, 0, depth);
            }
            if (node.max_value >= 0.0) {
                addHardIf(builder, true, kBiomeCoveragePct, CompareOp::Le, node.max_value, biomeArg, 0, 0, depth);
            }
            if (node.value >= 1.0) {
                addHardIf(builder, true, kBiomeContiguousRadius, CompareOp::Ge, node.value, biomeArg, 0, 0, depth);
            }
            if (noneOf && !node.values.empty()) {
                addHardOrBiomes(builder, kDominantBiome, CompareOp::Ne, node.values, depth);
            }
            return;
        }
        const CompareOp op = noneOf ? CompareOp::Ne : CompareOp::Eq;
        addHardOrBiomes(builder, kDominantBiome, op, node.values, depth);
        return;
    }

    if (node.kind == RuleNodeKind::BlockConstraint) {
        if (node.metric == "spawn_surface_block_id" && node.block_id >= 0) {
            const CompareOp op = node.op == "ne" ? CompareOp::Ne : CompareOp::Eq;
            addHardIf(builder, true, kSpawnSurfaceBlock, op, node.block_id, 0, 0, 0, depth);
        }
        return;
    }

    if (node.kind == RuleNodeKind::MetricObjective) {
        const MetricDef* def = resolveMetricName(node.metric);
        if (def == nullptr || !metricAllowedAtDepth(*def, depth)) {
            return;
        }
        double scale = node.value;
        if (scale <= 0.0) {
            if (node.metric == "biome_coverage_percent" || node.metric == "biome_blob_compactness_percent") {
                scale = 100.0;
            } else if (node.metric == "max_contiguous_biome_radius") {
                scale = 8.0;
            } else {
                scale = 20.0;
            }
        }
        const ObjectiveDir dir = node.direction == "minimize" ? ObjectiveDir::Minimize : ObjectiveDir::Maximize;
        const float weight = node.weight > 0.f ? node.weight : 1.f;
        const std::uint8_t biomeArg = primaryBiomeArg(node.values);
        builder.addObjective(def->id, weight, 0.0, scale, dir, biomeArg);
    }
}

void buildGraphFromLegacy(const SearchConfig& legacy, GraphBuilder& builder)
{
    const SeedProbeDepth depth = legacy.probe_depth_max;

    if (!legacy.spawn_biome.empty()) {
        addHardIf(builder, true, kSpawnBiome, CompareOp::Eq,
            static_cast<double>(biomeIdFromName(legacy.spawn_biome)), 0, 0, 0, depth);
    }
    if (!legacy.spawn_biome_in.empty()) {
        addHardOrBiomes(builder, kSpawnBiome, CompareOp::Eq, legacy.spawn_biome_in, depth);
    }
    if (legacy.spawn_x >= 0) {
        addHardIf(builder, true, kSpawnX, CompareOp::Eq, legacy.spawn_x, 0, 0, 0, depth);
    }
    if (legacy.spawn_z >= 0) {
        addHardIf(builder, true, kSpawnZ, CompareOp::Eq, legacy.spawn_z, 0, 0, 0, depth);
    }
    if (!legacy.spawn_surface_block.empty()) {
        const int blockId = blockIdFromName(legacy.spawn_surface_block);
        if (blockId >= 0) {
            addHardIf(builder, true, kSpawnSurfaceBlock, CompareOp::Eq, blockId, 0, 0, 0, depth);
        }
    }
    if (legacy.unique_biome_min >= 0) {
        addHardIf(builder, true, kUniqueBiomeCount, CompareOp::Ge, legacy.unique_biome_min, 0, 0, 0, depth);
    }
    if (legacy.unique_biome_max >= 0) {
        addHardIf(builder, true, kUniqueBiomeCount, CompareOp::Le, legacy.unique_biome_max, 0, 0, 0, depth);
    }
    if (!legacy.biome_at_required.empty()) {
        addHardIf(builder, true, kBiomeAt, CompareOp::Eq,
            static_cast<double>(biomeIdFromName(legacy.biome_at_required)),
            0, legacy.biome_at_x, legacy.biome_at_z, depth);
    }
    if (legacy.temperature_min >= 0.f) {
        addHardIf(builder, true, kAvgTemperature, CompareOp::Ge, legacy.temperature_min, 0, 0, 0, depth);
    }
    if (legacy.temperature_max >= 0.f) {
        addHardIf(builder, true, kAvgTemperature, CompareOp::Le, legacy.temperature_max, 0, 0, 0, depth);
    }
    if (legacy.downfall_min >= 0.f) {
        addHardIf(builder, true, kAvgDownfall, CompareOp::Ge, legacy.downfall_min, 0, 0, 0, depth);
    }
    if (legacy.downfall_max >= 0.f) {
        addHardIf(builder, true, kAvgDownfall, CompareOp::Le, legacy.downfall_max, 0, 0, 0, depth);
    }
    if (legacy.avg_surface_y_min >= 0.f) {
        addHardIf(builder, true, kAvgSurfaceY, CompareOp::Ge, legacy.avg_surface_y_min, 0, 0, 0, depth);
    }
    if (legacy.avg_surface_y_max >= 0.f) {
        addHardIf(builder, true, kAvgSurfaceY, CompareOp::Le, legacy.avg_surface_y_max, 0, 0, 0, depth);
    }
    if (legacy.flatness_max >= 0.f) {
        addHardIf(builder, true, kFlatness, CompareOp::Le, legacy.flatness_max, 0, 0, 0, depth);
    }
    if (legacy.underwater_percent_max >= 0.f) {
        addHardIf(builder, true, kUnderwaterPct, CompareOp::Le, legacy.underwater_percent_max, 0, 0, 0, depth);
    }

    const std::vector<std::string>& areaBiomes =
        !legacy.required_biomes.empty() ? legacy.required_biomes
        : (!legacy.spawn_biome.empty() ? std::vector<std::string>{legacy.spawn_biome} : std::vector<std::string>{});
    for (const std::string& biomeName : areaBiomes) {
        const std::uint8_t biomeArg = biomeIdFromName(biomeName);
        if (legacy.required_biome_min_coverage_percent >= 0.f) {
            addHardIf(builder, true, kBiomeCoveragePct, CompareOp::Ge,
                legacy.required_biome_min_coverage_percent, biomeArg, 0, 0, depth);
        }
        if (legacy.required_biome_min_contiguous_radius_chunks >= 1) {
            addHardIf(builder, true, kBiomeContiguousRadius, CompareOp::Ge,
                legacy.required_biome_min_contiguous_radius_chunks, biomeArg, 0, 0, depth);
        }
        if (legacy.required_biome_min_compactness_percent >= 0.f) {
            addHardIf(builder, true, kBiomeBlobCompactness, CompareOp::Ge,
                legacy.required_biome_min_compactness_percent, biomeArg, 0, 0, depth);
        }
    }
    for (const std::string& forbidden : legacy.forbidden_biomes) {
        const std::uint8_t biomeArg = biomeIdFromName(forbidden);
        if (legacy.forbidden_biome_max_coverage_percent >= 0.f) {
            addHardIf(builder, true, kBiomeCoveragePct, CompareOp::Le,
                legacy.forbidden_biome_max_coverage_percent, biomeArg, 0, 0, depth);
        } else {
            addHardIf(builder, true, kBiomeCoveragePct, CompareOp::Le, 0.0, biomeArg, 0, 0, depth);
        }
    }

    for (const RuleNode& node : legacy.advanced_rule_nodes) {
        applyRuleNode(builder, node, depth);
    }
}

} // namespace

ScanConfig toScanConfig(const SearchConfig& legacy)
{
    ScanConfig cfg;
    cfg.seed_start = legacy.seed_start;
    cfg.seed_end = legacy.seed_end;
    cfg.scan_origin_chunk_x = legacy.scan_origin_x >> 4;
    cfg.scan_origin_chunk_z = legacy.scan_origin_z >> 4;
    cfg.scan_radius_chunks = std::max(1, legacy.scan_radius_chunks);
    cfg.threads = std::max(1, legacy.threads);
    cfg.top_k = std::max(1, legacy.top_k);
    cfg.checkpoint_path = legacy.checkpoint_path;

    GraphBuilder builder;
    buildGraphFromLegacy(legacy, builder);
    cfg.graph = builder.finish();
    return cfg;
}

} // namespace seedfinder::config
