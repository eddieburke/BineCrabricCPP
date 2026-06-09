#pragma once

#include "seedfinder/graph/NodeGraph.hpp"

#include <vector>

namespace seedfinder::graph {

// Small helper for assembling a NodeGraph: wires hard constraints into an And node
// feeding the Result inlet, and objective nodes into the ranking inlet.
class GraphBuilder {
public:
    GraphBuilder();

    void addHardCompare(int metricId, CompareOp op, double threshold,
        std::uint8_t biomeArg = 0, int argX = 0, int argZ = 0);

    // Satisfied when any branch passes (e.g. spawn is one of several biomes).
    void addHardOrCompare(int metricId, CompareOp op, const std::vector<double>& thresholds,
        std::uint8_t biomeArg = 0, int argX = 0, int argZ = 0);

    void addObjective(int metricId, float weight, double normMin, double normMax,
        ObjectiveDir dir = ObjectiveDir::Maximize,
        std::uint8_t biomeArg = 0, int argX = 0, int argZ = 0);

    [[nodiscard]] NodeGraph finish();

private:
    NodeGraph graph_;
    int resultIndex_ = -1;
    int andIndex_ = -1;

    [[nodiscard]] int addCompareChain(int metricId, CompareOp op, double threshold,
        std::uint8_t biomeArg, int argX, int argZ);
    void ensureAnd();
    void wireHard(int boolNodeIndex);
};

} // namespace seedfinder::graph
