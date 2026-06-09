#include "seedfinder/engine/Scorer.hpp"

#include "seedfinder/engine/BiomeCatalog.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <queue>
#include <vector>

namespace seedfinder::engine {
namespace {

bool listContainsBiome(const std::vector<std::string>& names, std::uint8_t biome_id)
{
    for (const std::string& name : names) {
        if (biomeMatchesName(biome_id, name)) {
            return true;
        }
    }
    return false;
}

void addSchemaOnlyNotes(const config::SearchConfig& cfg, ScoreResult& out)
{
    for (const std::string& key : cfg.schema_only_keys) {
        out.not_evaluated.push_back(key);
    }
}

float normalizeRankMetric(float value, float scale)
{
    if (scale <= 0.f) {
        return 0.f;
    }
    const float normalized = value / scale;
    return normalized > 1.f ? 1.f : normalized;
}

struct BiomeAreaMetrics {
    double coverage_percent = 0.0;
    int matching_chunks = 0;
    int total_chunks = 0;
    int max_contiguous_radius = 0;
    int max_contiguous_chunk_count = 0;
    double blob_compactness_percent = 0.0;
};

bool chunkMatchesBiomeList(std::uint8_t biome_id, const std::vector<std::string>& names, bool none_of)
{
    const bool matched = listContainsBiome(names, biome_id);
    return none_of ? !matched : matched;
}

BiomeAreaMetrics computeBiomeAreaMetrics(
    const SeedProbeResult& result,
    const std::vector<std::string>& biome_names,
    bool none_of)
{
    BiomeAreaMetrics metrics;
    if (result.biome_grid == nullptr || result.biome_grid_len == 0 || biome_names.empty()) {
        return metrics;
    }

    const int side = static_cast<int>(std::lround(std::sqrt(static_cast<double>(result.biome_grid_len))));
    if (side <= 0 || static_cast<std::uint32_t>(side * side) != result.biome_grid_len) {
        return metrics;
    }

    metrics.total_chunks = side * side;
    std::vector<std::uint8_t> match(static_cast<std::size_t>(metrics.total_chunks), 0);
    for (int i = 0; i < metrics.total_chunks; ++i) {
        if (chunkMatchesBiomeList(result.biome_grid[i].biome_id, biome_names, none_of)) {
            match[static_cast<std::size_t>(i)] = 1;
            ++metrics.matching_chunks;
        }
    }

    metrics.coverage_percent = metrics.total_chunks > 0
        ? (static_cast<double>(metrics.matching_chunks) * 100.0 / static_cast<double>(metrics.total_chunks))
        : 0.0;

    std::vector<std::uint8_t> visited(match.size(), 0);
    const int dx[4] = {1, -1, 0, 0};
    const int dz[4] = {0, 0, 1, -1};
    for (int start = 0; start < metrics.total_chunks; ++start) {
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
        if (componentRadius > metrics.max_contiguous_radius) {
            metrics.max_contiguous_radius = componentRadius;
            metrics.max_contiguous_chunk_count = componentSize;
        }
    }

    if (metrics.matching_chunks > 0 && metrics.max_contiguous_chunk_count > 0) {
        metrics.blob_compactness_percent =
            static_cast<double>(metrics.max_contiguous_chunk_count) * 100.0
            / static_cast<double>(metrics.matching_chunks);
    }

    return metrics;
}

bool biomeGridHasBiome(const SeedProbeResult& result, const std::string& name)
{
    if (result.biome_grid == nullptr) {
        return false;
    }
    for (std::uint32_t i = 0; i < result.biome_grid_len; ++i) {
        if (biomeMatchesName(result.biome_grid[i].biome_id, name)) {
            return true;
        }
    }
    return false;
}

double biomeCoveragePercent(const SeedProbeResult& result, const std::string& name)
{
    if (result.biome_grid == nullptr || result.biome_grid_len == 0) {
        return 0.0;
    }
    const BiomeAreaMetrics metrics = computeBiomeAreaMetrics(result, {name}, false);
    return metrics.coverage_percent;
}

int biomeContiguousRadius(const SeedProbeResult& result, const std::string& name)
{
    if (result.biome_grid == nullptr || result.biome_grid_len == 0) {
        return 0;
    }
    const BiomeAreaMetrics metrics = computeBiomeAreaMetrics(result, {name}, false);
    return metrics.max_contiguous_radius;
}

double biomeBlobCompactnessPercent(const SeedProbeResult& result, const std::string& name)
{
    if (result.biome_grid == nullptr || result.biome_grid_len == 0) {
        return 0.0;
    }
    const BiomeAreaMetrics metrics = computeBiomeAreaMetrics(result, {name}, false);
    return metrics.blob_compactness_percent;
}

bool computeClimateAverages(const SeedProbeResult& result, float& avgTemperature, float& avgDownfall)
{
    if (result.biome_grid == nullptr || result.biome_grid_len == 0) {
        return false;
    }
    double tempSum = 0.0;
    double downfallSum = 0.0;
    for (std::uint32_t i = 0; i < result.biome_grid_len; ++i) {
        tempSum += static_cast<double>(result.biome_grid[i].temperature);
        downfallSum += static_cast<double>(result.biome_grid[i].downfall);
    }
    const double count = static_cast<double>(result.biome_grid_len);
    avgTemperature = static_cast<float>(tempSum / count);
    avgDownfall = static_cast<float>(downfallSum / count);
    return true;
}

float readMetricValue(
    const SeedProbeResult& result,
    const std::string& metricName,
    const std::vector<std::string>& biomeValues)
{
    if (metricName == "max_cactus_height") {
        return static_cast<float>(result.max_cactus_height);
    }
    if (metricName == "unique_biome_count") {
        return static_cast<float>(result.unique_biome_count);
    }
    if (metricName == "dominant_biome_percent") {
        return static_cast<float>(result.dominant_biome_percent);
    }
    if (metricName == "biome_chunk_count" || metricName == "desert_chunk_count") {
        if (!biomeValues.empty()) {
            return static_cast<float>(biomeChunkCount(result, biomeValues.front()));
        }
        if (metricName == "desert_chunk_count") {
            return static_cast<float>(result.desert_chunk_count);
        }
        return -1.f;
    }
    const std::string_view chunkBiome = chunkCountBiomeFromMetric(metricName);
    if (!chunkBiome.empty()) {
        return static_cast<float>(biomeChunkCount(result, chunkBiome));
    }
    return -1.f;
}

bool evaluateRuleConstraint(const config::RuleNode& node, const SeedProbeResult& result, ScoreResult& out)
{
    if (node.kind == config::RuleNodeKind::BiomeConstraint) {
        if (node.values.empty()) {
            return true;
        }
        const std::string mode = node.op.empty() ? "any_of" : node.op;
        const bool none_of = mode == "none_of";
        const bool areaMode = node.min_value >= 0.0 || node.max_value >= 0.0 || node.value >= 1.0;
        if (areaMode) {
            if (result.biome_grid == nullptr || result.biome_grid_len == 0) {
                out.notes.push_back("rule biome_constraint area mode requires biome grid");
                return false;
            }
            const BiomeAreaMetrics metrics = computeBiomeAreaMetrics(result, node.values, none_of);
            if (!none_of && metrics.matching_chunks == 0) {
                out.notes.push_back("rule biome_constraint area mode found no matching chunks");
                return false;
            }
            if (node.min_value >= 0.0 && metrics.coverage_percent < node.min_value) {
                out.notes.push_back("rule biome_constraint min_coverage_percent failed");
                return false;
            }
            if (node.max_value >= 0.0 && metrics.coverage_percent > node.max_value) {
                out.notes.push_back("rule biome_constraint max_coverage_percent failed");
                return false;
            }
            if (node.value >= 1.0 && metrics.max_contiguous_radius < static_cast<int>(node.value)) {
                out.notes.push_back("rule biome_constraint min_contiguous_radius_chunks failed");
                return false;
            }
            return true;
        }
        if (none_of) {
            for (const std::string& biome : node.values) {
                if (biomeMatchesName(result.dominant_biome_id, biome)) {
                    out.notes.push_back("rule biome_constraint none_of failed");
                    return false;
                }
            }
            return true;
        }
        for (const std::string& biome : node.values) {
            if (biomeMatchesName(result.dominant_biome_id, biome)) {
                return true;
            }
        }
        out.notes.push_back("rule biome_constraint any_of failed");
        return false;
    }
    if (node.kind == config::RuleNodeKind::BlockConstraint) {
        if (node.metric == "spawn_surface_block_id") {
            if (!result.spawn.valid || node.block_id < 0) {
                return false;
            }
            const int id = static_cast<int>(result.spawn.surface_block_id);
            if ((node.op == "ne" && id == node.block_id) || (node.op != "ne" && id != node.block_id)) {
                out.notes.push_back("rule block_constraint spawn_surface_block_id failed");
                return false;
            }
            return true;
        }
        if (node.metric == "block_histogram" && node.block_id >= 0) {
            const std::size_t idx = static_cast<std::size_t>(node.block_id) & 0xFFu;
            const double count = static_cast<double>(result.block_histogram[idx]);
            if (node.min_value >= 0.0 && count < node.min_value) {
                out.notes.push_back("rule block_constraint block_histogram min failed");
                return false;
            }
            if (node.max_value >= 0.0 && count > node.max_value) {
                out.notes.push_back("rule block_constraint block_histogram max failed");
                return false;
            }
            return true;
        }
        out.not_evaluated.push_back("rule block_constraint unsupported metric: " + node.metric);
        return true;
    }
    if (node.kind == config::RuleNodeKind::BlockOnBlock) {
        out.not_evaluated.push_back("rule block_on_block not yet implemented");
        return true;
    }
    return true;
}

float computeRuleObjectiveScore(const config::SearchConfig& cfg, const SeedProbeResult& result, ScoreResult& out)
{
    float weighted = 0.f;
    float totalWeight = 0.f;
    const int radius = std::max(1, cfg.scan_radius_chunks);
    const float totalChunks = static_cast<float>((2 * radius + 1) * (2 * radius + 1));
    for (const config::RuleNode& node : cfg.advanced_rule_nodes) {
        if (node.kind != config::RuleNodeKind::MetricObjective) {
            continue;
        }
        if (result.depth_reached < kDepthFullDecorate && node.metric == "max_cactus_height") {
            out.not_evaluated.push_back("rule metric_objective requires full_decorate: " + node.metric);
            continue;
        }
        float scale = static_cast<float>(node.value);
        if (scale <= 0.f) {
            if (node.metric == "desert_chunk_count" || node.metric == "biome_chunk_count"
                || !chunkCountBiomeFromMetric(node.metric).empty()) {
                scale = totalChunks;
            } else if (node.metric == "max_contiguous_biome_radius") {
                scale = static_cast<float>(radius);
            } else if (node.metric == "biome_blob_compactness_percent"
                || node.metric == "biome_coverage_percent" || node.metric == "dominant_biome_percent") {
                scale = 100.f;
            } else {
                scale = 20.f;
            }
        }
        float metric = -1.f;
        if (node.metric == "biome_coverage_percent" || node.metric == "max_contiguous_biome_radius"
            || node.metric == "biome_blob_compactness_percent") {
            if (node.values.empty() || result.biome_grid == nullptr || result.biome_grid_len == 0) {
                out.not_evaluated.push_back("rule metric_objective requires biomes: " + node.metric);
                continue;
            }
            const BiomeAreaMetrics areaMetrics = computeBiomeAreaMetrics(result, node.values, false);
            if (node.metric == "biome_coverage_percent") {
                metric = static_cast<float>(areaMetrics.coverage_percent);
            } else if (node.metric == "max_contiguous_biome_radius") {
                metric = static_cast<float>(areaMetrics.max_contiguous_radius);
            } else {
                metric = static_cast<float>(areaMetrics.blob_compactness_percent);
            }
        } else {
            metric = readMetricValue(result, node.metric, node.values);
        }
        if (metric < 0.f) {
            out.not_evaluated.push_back("rule metric_objective unknown metric: " + node.metric);
            continue;
        }
        float normalized = normalizeRankMetric(metric, scale);
        if (node.direction == "minimize") {
            normalized = 1.f - normalized;
        }
        const float weight = node.weight > 0.f ? node.weight : 1.f;
        weighted += normalized * weight;
        totalWeight += weight;
    }
    return totalWeight > 0.f ? (weighted / totalWeight) : 0.f;
}

} // namespace

ScoreResult scoreResult(const config::SearchConfig& cfg, const SeedProbeResult& result)
{
    ScoreResult out;
    out.search_tier_passed = result.depth_reached;
    addSchemaOnlyNotes(cfg, out);

    float earned = 0.f;
    float possible = 0.f;

    if (!cfg.spawn_biome.empty()) {
        ++possible;
        if (result.spawn.valid && biomeMatchesName(result.spawn.biome_id, cfg.spawn_biome)) {
            earned += 1.f;
        } else {
            out.notes.push_back("spawn_biome mismatch");
        }
    }
    if (!cfg.spawn_biome_in.empty()) {
        ++possible;
        if (result.spawn.valid && listContainsBiome(cfg.spawn_biome_in, result.spawn.biome_id)) {
            earned += 1.f;
        } else {
            out.notes.push_back("spawn_biome_in mismatch");
        }
    }
    if (cfg.spawn_x >= 0) {
        ++possible;
        if (result.spawn.valid && result.spawn.x == cfg.spawn_x) {
            earned += 1.f;
        }
    }
    if (cfg.spawn_z >= 0) {
        ++possible;
        if (result.spawn.valid && result.spawn.z == cfg.spawn_z) {
            earned += 1.f;
        }
    }
    if (!cfg.spawn_surface_block.empty() && cfg.spawn_surface_block == "sand") {
        ++possible;
        if (result.spawn.on_sand_beach) {
            earned += 1.f;
        }
    }
    if (cfg.unique_biome_min >= 0) {
        ++possible;
        if (static_cast<int>(result.unique_biome_count) >= cfg.unique_biome_min) {
            earned += 1.f;
        }
    }
    if (cfg.unique_biome_max >= 0) {
        ++possible;
        if (static_cast<int>(result.unique_biome_count) <= cfg.unique_biome_max) {
            earned += 1.f;
        }
    }
    if (cfg.avg_surface_y_min >= 0.f) {
        ++possible;
        if (result.avg_surface_y >= cfg.avg_surface_y_min) {
            earned += 1.f;
        }
    }
    if (cfg.avg_surface_y_max >= 0.f) {
        ++possible;
        if (result.avg_surface_y <= cfg.avg_surface_y_max) {
            earned += 1.f;
        }
    }
    if (cfg.underwater_percent_max >= 0.f) {
        ++possible;
        if (result.underwater_percent <= cfg.underwater_percent_max) {
            earned += 1.f;
        }
    }
    if (!cfg.required_biomes.empty()) {
        ++possible;
        bool ok = true;
        for (const std::string& required : cfg.required_biomes) {
            bool found = biomeGridHasBiome(result, required);
            if (!found && biomeMatchesName(result.dominant_biome_id, required)) {
                found = true;
            }
            if (!found) {
                ok = false;
                break;
            }
            if (cfg.required_biome_min_coverage_percent >= 0.f) {
                if (biomeCoveragePercent(result, required) < static_cast<double>(cfg.required_biome_min_coverage_percent)) {
                    ok = false;
                    break;
                }
            }
            if (cfg.required_biome_min_contiguous_radius_chunks >= 1) {
                if (biomeContiguousRadius(result, required) < cfg.required_biome_min_contiguous_radius_chunks) {
                    ok = false;
                    break;
                }
            }
            if (cfg.required_biome_min_compactness_percent >= 0.f) {
                if (biomeBlobCompactnessPercent(result, required)
                    < static_cast<double>(cfg.required_biome_min_compactness_percent)) {
                    ok = false;
                    break;
                }
            }
        }
        if (ok) {
            earned += 1.f;
        }
    }
    if (!cfg.forbidden_biomes.empty()) {
        ++possible;
        bool ok = true;
        if (cfg.forbidden_biome_max_coverage_percent >= 0.f) {
            for (const std::string& forbidden : cfg.forbidden_biomes) {
                if (biomeCoveragePercent(result, forbidden)
                    > static_cast<double>(cfg.forbidden_biome_max_coverage_percent)) {
                    ok = false;
                    break;
                }
            }
        } else if (listContainsBiome(cfg.forbidden_biomes, result.dominant_biome_id)) {
            ok = false;
        }
        if (ok) {
            earned += 1.f;
        }
    }
    if (cfg.temperature_min >= 0.f || cfg.temperature_max >= 0.f || cfg.downfall_min >= 0.f || cfg.downfall_max >= 0.f) {
        float avgTemp = 0.f;
        float avgDownfall = 0.f;
        if (computeClimateAverages(result, avgTemp, avgDownfall)) {
            if (cfg.temperature_min >= 0.f) {
                ++possible;
                if (avgTemp >= cfg.temperature_min) {
                    earned += 1.f;
                }
            }
            if (cfg.temperature_max >= 0.f) {
                ++possible;
                if (avgTemp <= cfg.temperature_max) {
                    earned += 1.f;
                }
            }
            if (cfg.downfall_min >= 0.f) {
                ++possible;
                if (avgDownfall >= cfg.downfall_min) {
                    earned += 1.f;
                }
            }
            if (cfg.downfall_max >= 0.f) {
                ++possible;
                if (avgDownfall <= cfg.downfall_max) {
                    earned += 1.f;
                }
            }
        }
    }

    for (const std::string& key : cfg.schema_only_keys) {
        const config::ParamDescriptor* desc = config::findParamByKey(key);
        if (desc != nullptr && cfg.raw_values.count(key) > 0) {
            const std::string& raw = cfg.raw_values.at(key);
            if (raw != "0" && raw != "false" && !raw.empty()) {
                out.notes.push_back("schema-only constraint active but not evaluated: " + key);
            }
        }
    }

    for (const config::RuleNode& node : cfg.advanced_rule_nodes) {
        if (node.kind == config::RuleNodeKind::MetricObjective) {
            continue;
        }
        ++possible;
        if (evaluateRuleConstraint(node, result, out)) {
            earned += 1.f;
        }
    }
    for (const std::string& error : cfg.advanced_rule_errors) {
        out.not_evaluated.push_back(error);
    }

    out.partial_match_score = possible > 0.f ? (earned / possible) : 1.f;
    if (!cfg.advanced_rule_nodes.empty()) {
        const float objectiveScore = computeRuleObjectiveScore(cfg, result, out);
        if (possible > 0.f) {
            out.partial_match_score = (out.partial_match_score + objectiveScore) * 0.5f;
        } else if (objectiveScore > 0.f) {
            out.partial_match_score = objectiveScore;
        }
    }
    out.all_hard_constraints_met = out.notes.empty() && out.not_evaluated.empty() && earned >= possible;
    if (possible == 0.f) {
        out.all_hard_constraints_met = true;
    }
    return out;
}

} // namespace seedfinder::engine
