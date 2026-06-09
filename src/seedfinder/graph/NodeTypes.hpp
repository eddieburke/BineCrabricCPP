#pragma once

#include <cstdint>

// Node kinds and port typing for the seed-finder node graph. A graph is a small DAG:
// Metric/Constant sources feed Compare/Logic nodes (producing Bool), which feed the
// single Result node's hard-constraint inlet; Objective nodes feed its ranking inlet.

namespace seedfinder::graph {

enum class NodeKind : std::uint8_t {
    Metric,    // 0 inputs, 1 Value out  — emits a metric value (params: metricId + args)
    Constant,  // 0 inputs, 1 Value out  — emits params.constant
    Compare,   // 1 Value in, 1 Bool out — (in op constant)
    And,       // Bool fan-in, 1 Bool out
    Or,        // Bool fan-in, 1 Bool out
    Not,       // 1 Bool in, 1 Bool out
    Objective, // 1 Value in, 1 Value out — normalized, weighted score contribution
    Result,    // inlet 0 = hard Bool fan-in, inlet 1 = Objective Value fan-in; no out
};

enum class PortType : std::uint8_t { Value, Bool };

enum class CompareOp : std::uint8_t { Ge, Le, Gt, Lt, Eq, Ne };

enum class ObjectiveDir : std::uint8_t { Maximize, Minimize };

// Result inlet ports.
constexpr int kResultHardPort = 0;
constexpr int kResultObjectivePort = 1;

[[nodiscard]] constexpr int nodeInputCount(NodeKind kind)
{
    switch (kind) {
    case NodeKind::Metric:
    case NodeKind::Constant:
        return 0;
    case NodeKind::Compare:
    case NodeKind::Not:
    case NodeKind::Objective:
        return 1;
    case NodeKind::And:
    case NodeKind::Or:
        return 1; // single fan-in inlet
    case NodeKind::Result:
        return 2; // hard + objective inlets
    }
    return 0;
}

[[nodiscard]] constexpr bool nodeHasOutput(NodeKind kind)
{
    return kind != NodeKind::Result;
}

[[nodiscard]] constexpr PortType nodeOutputType(NodeKind kind)
{
    switch (kind) {
    case NodeKind::Metric:
    case NodeKind::Constant:
    case NodeKind::Objective:
        return PortType::Value;
    default:
        return PortType::Bool;
    }
}

[[nodiscard]] constexpr PortType nodeInputType(NodeKind kind, int port)
{
    switch (kind) {
    case NodeKind::Compare:
    case NodeKind::Objective:
        return PortType::Value;
    case NodeKind::And:
    case NodeKind::Or:
    case NodeKind::Not:
        return PortType::Bool;
    case NodeKind::Result:
        return port == kResultObjectivePort ? PortType::Value : PortType::Bool;
    default:
        return PortType::Value;
    }
}

[[nodiscard]] constexpr const char* nodeKindName(NodeKind kind)
{
    switch (kind) {
    case NodeKind::Metric: return "Metric";
    case NodeKind::Constant: return "Constant";
    case NodeKind::Compare: return "Compare";
    case NodeKind::And: return "And";
    case NodeKind::Or: return "Or";
    case NodeKind::Not: return "Not";
    case NodeKind::Objective: return "Objective";
    case NodeKind::Result: return "Result";
    }
    return "?";
}

[[nodiscard]] constexpr const char* compareOpSymbol(CompareOp op)
{
    switch (op) {
    case CompareOp::Ge: return ">=";
    case CompareOp::Le: return "<=";
    case CompareOp::Gt: return ">";
    case CompareOp::Lt: return "<";
    case CompareOp::Eq: return "==";
    case CompareOp::Ne: return "!=";
    }
    return "?";
}

[[nodiscard]] constexpr bool applyCompare(CompareOp op, double a, double b)
{
    switch (op) {
    case CompareOp::Ge: return a >= b;
    case CompareOp::Le: return a <= b;
    case CompareOp::Gt: return a > b;
    case CompareOp::Lt: return a < b;
    case CompareOp::Eq: return a == b;
    case CompareOp::Ne: return a != b;
    }
    return false;
}

} // namespace seedfinder::graph
