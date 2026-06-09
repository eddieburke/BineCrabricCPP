#pragma once

#include "seedfinder/probe/ProbeResult.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

// The metric registry: every value a node graph can read from a probe. Each metric
// declares the generation passes it needs (unioned by the planner) and whether it
// computes a spawn point. Replaces the old 84-entry ConfigSchema, all real this time.

namespace seedfinder::metric {

enum class ValueKind : std::uint8_t {
    Int,
    Float,
    Percent,
    Biome,   // value is a biome id; compare against a biome-picker constant
    BlockId, // value is a block id
};

struct MetricArgs {
    std::uint8_t biome = 0; // for takesBiome metrics
    int x = 0;              // for takesXZ metrics
    int z = 0;
};

struct MetricDef {
    int id = 0;
    const char* name = "";    // stable snake_case key (serialization)
    const char* display = ""; // UI label
    const char* category = "";
    ValueKind kind = ValueKind::Int;
    std::uint32_t requiredPasses = 0;
    bool needsSpawn = false;
    bool takesBiome = false;
    bool takesXZ = false;
};

[[nodiscard]] const std::vector<MetricDef>& allMetrics();
[[nodiscard]] const MetricDef* metricById(int id);
[[nodiscard]] const MetricDef* metricByName(std::string_view name);

// True if any metric reachable in a graph (caller-supplied list of ids) needs spawn.
[[nodiscard]] bool metricNeedsSpawn(int id);

[[nodiscard]] double evalMetric(int id, const MetricArgs& args, const ProbeResult& r);

} // namespace seedfinder::metric
