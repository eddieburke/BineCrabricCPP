#include "seedfinder/graph/GraphBuilder.hpp"

namespace seedfinder::graph {

GraphBuilder::GraphBuilder()
{
    Node result;
    result.kind = NodeKind::Result;
    resultIndex_ = graph_.addNode(result);
}

void GraphBuilder::ensureAnd()
{
    if (andIndex_ >= 0) {
        return;
    }
    Node andNode;
    andNode.kind = NodeKind::And;
    andIndex_ = graph_.addNode(andNode);
    graph_.connect(andIndex_, 0, resultIndex_, kResultHardPort);
}

void GraphBuilder::wireHard(int boolNodeIndex)
{
    ensureAnd();
    graph_.connect(boolNodeIndex, 0, andIndex_, 0);
}

int GraphBuilder::addCompareChain(int metricId, CompareOp op, double threshold,
    std::uint8_t biomeArg, int argX, int argZ)
{
    Node metric;
    metric.kind = NodeKind::Metric;
    metric.metricId = metricId;
    metric.biomeArg = biomeArg;
    metric.argX = argX;
    metric.argZ = argZ;
    const int metricIndex = graph_.addNode(metric);

    Node compare;
    compare.kind = NodeKind::Compare;
    compare.op = op;
    compare.constant = threshold;
    const int compareIndex = graph_.addNode(compare);

    graph_.connect(metricIndex, 0, compareIndex, 0);
    return compareIndex;
}

void GraphBuilder::addHardCompare(int metricId, CompareOp op, double threshold,
    std::uint8_t biomeArg, int argX, int argZ)
{
    wireHard(addCompareChain(metricId, op, threshold, biomeArg, argX, argZ));
}

void GraphBuilder::addHardOrCompare(int metricId, CompareOp op, const std::vector<double>& thresholds,
    std::uint8_t biomeArg, int argX, int argZ)
{
    if (thresholds.empty()) {
        return;
    }
    if (thresholds.size() == 1) {
        addHardCompare(metricId, op, thresholds.front(), biomeArg, argX, argZ);
        return;
    }
    Node orNode;
    orNode.kind = NodeKind::Or;
    const int orIndex = graph_.addNode(orNode);
    for (double threshold : thresholds) {
        const int compareIndex = addCompareChain(metricId, op, threshold, biomeArg, argX, argZ);
        graph_.connect(compareIndex, 0, orIndex, 0);
    }
    wireHard(orIndex);
}

void GraphBuilder::addObjective(int metricId, float weight, double normMin, double normMax,
    ObjectiveDir dir, std::uint8_t biomeArg, int argX, int argZ)
{
    Node metric;
    metric.kind = NodeKind::Metric;
    metric.metricId = metricId;
    metric.biomeArg = biomeArg;
    metric.argX = argX;
    metric.argZ = argZ;
    const int metricIndex = graph_.addNode(metric);

    Node objective;
    objective.kind = NodeKind::Objective;
    objective.objectiveDir = dir;
    objective.weight = weight;
    objective.normMin = normMin;
    objective.normMax = normMax;
    const int objectiveIndex = graph_.addNode(objective);

    graph_.connect(metricIndex, 0, objectiveIndex, 0);
    graph_.connect(objectiveIndex, 0, resultIndex_, kResultObjectivePort);
}

NodeGraph GraphBuilder::finish()
{
    return graph_;
}

} // namespace seedfinder::graph
