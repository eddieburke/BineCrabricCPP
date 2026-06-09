#pragma once

#include "seedfinder/graph/NodeTypes.hpp"
#include "seedfinder/probe/ProbeResult.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace seedfinder::graph {

// One node. Only the params relevant to its kind are used; the rest are ignored.
struct Node {
    NodeKind kind = NodeKind::Metric;

    // Metric source
    int metricId = -1;
    std::uint8_t biomeArg = 0; // biome id for biome-keyed metrics
    int argX = 0;              // world coords for positional metrics (biome_at)
    int argZ = 0;

    // Constant / Compare
    double constant = 0.0;
    CompareOp op = CompareOp::Ge;

    // Objective
    ObjectiveDir objectiveDir = ObjectiveDir::Maximize;
    float weight = 1.0f;
    double normMin = 0.0; // value mapped to 0
    double normMax = 1.0; // value mapped to 1

    // Editor layout (canvas position)
    float posX = 0.0f;
    float posY = 0.0f;
};

struct Connection {
    int fromNode = -1;
    int fromPort = 0;
    int toNode = -1;
    int toPort = 0;
};

// Result of evaluating the graph against one probe. hardPass is false only when a hard
// constraint that is *fully determined by the available passes* is violated, so it is
// safe for tiered early-out: unknown (not-yet-probed) branches never force a rejection.
struct GraphVerdict {
    bool hardPass = false;
    float score = 0.0f;
};

class NodeGraph {
public:
    std::vector<Node> nodes;
    std::vector<Connection> connections;

    [[nodiscard]] int addNode(const Node& node);
    void removeNode(int index);                 // also drops its connections, reindexes
    void connect(int fromNode, int fromPort, int toNode, int toPort);
    void disconnect(int fromNode, int fromPort, int toNode, int toPort);
    void disconnectInlet(int toNode, int toPort); // drop every wire into one inlet
    [[nodiscard]] int findResultNode() const;   // -1 if none
    void clear();

    // Union of requiredPasses over every Metric node reachable from the Result node.
    // Returns 0 when there is no Result node (nothing to search for).
    [[nodiscard]] std::uint32_t requiredPasses() const;

    // Evaluates against `r`, treating only the passes in `availablePasses` as computed.
    // Metric branches that need other passes are "unknown": they never reject a hard
    // constraint and contribute 0 to the score. Pass the full pass mask for the final,
    // exact verdict.
    [[nodiscard]] GraphVerdict evaluate(const ProbeResult& r, std::uint32_t availablePasses) const;

    // Compact, round-trippable text serialization (one node/connection per line).
    [[nodiscard]] std::string serialize() const;
    bool deserialize(const std::string& blob);

private:
    struct EvalVal {
        double v = 0.0;
        bool known = true; // false => depends on a pass not yet computed
    };
    EvalVal evalNode(int index, const ProbeResult& r, std::uint32_t available,
        std::vector<EvalVal>& cache, std::vector<std::uint8_t>& state) const;
    void collectReachableMetrics(int node, std::vector<std::uint8_t>& seen, std::uint32_t& passes) const;
};

} // namespace seedfinder::graph
