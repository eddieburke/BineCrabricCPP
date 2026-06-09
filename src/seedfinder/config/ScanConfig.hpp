#pragma once

#include "seedfinder/engine/Pass.hpp"
#include "seedfinder/graph/NodeGraph.hpp"
#include "seedfinder/metric/Metric.hpp"

#include <cstdint>
#include <string>

// The full search description: seed range, scan region, threading/output knobs, and the
// node graph that defines what makes a seed a hit. Replaces the old SearchConfig +
// 84-param ConfigSchema.

namespace seedfinder::config {

struct ScanConfig {
    std::uint64_t seed_start = 0;
    std::uint64_t seed_end = 1000;

    int scan_origin_chunk_x = 0;
    int scan_origin_chunk_z = 0;
    int scan_radius_chunks = 4;

    int threads = 1;
    int top_k = 10;
    float min_score = 0.0f;
    std::string checkpoint_path;

    graph::NodeGraph graph;

    [[nodiscard]] bool graphNeedsSpawn() const
    {
        for (const graph::Node& n : graph.nodes) {
            if (n.kind == graph::NodeKind::Metric && metric::metricNeedsSpawn(n.metricId)) {
                return true;
            }
        }
        return false;
    }

    // Resolves the graph's metric requirements into a concrete pass plan. Always samples
    // biomes (so the preview map and biome metrics have data) even for an empty graph.
    [[nodiscard]] PassPlan plan() const
    {
        std::uint32_t passes = graph.requiredPasses();
        passes |= pass::Biome;
        return makePassPlan(passes, scan_radius_chunks, graphNeedsSpawn());
    }
};

} // namespace seedfinder::config
