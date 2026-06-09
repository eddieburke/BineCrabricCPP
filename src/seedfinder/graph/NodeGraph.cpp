#include "seedfinder/graph/NodeGraph.hpp"

#include "seedfinder/metric/Metric.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace seedfinder::graph {

int NodeGraph::addNode(const Node& node)
{
    nodes.push_back(node);
    return static_cast<int>(nodes.size()) - 1;
}

void NodeGraph::removeNode(int index)
{
    if (index < 0 || index >= static_cast<int>(nodes.size())) {
        return;
    }
    nodes.erase(nodes.begin() + index);
    // Drop connections touching the node, then shift indices above it down by one.
    std::vector<Connection> kept;
    kept.reserve(connections.size());
    for (Connection c : connections) {
        if (c.fromNode == index || c.toNode == index) {
            continue;
        }
        if (c.fromNode > index) {
            --c.fromNode;
        }
        if (c.toNode > index) {
            --c.toNode;
        }
        kept.push_back(c);
    }
    connections.swap(kept);
}

void NodeGraph::connect(int fromNode, int fromPort, int toNode, int toPort)
{
    // A single-input inlet keeps only the latest wire; fan-in inlets accumulate.
    const bool fanIn = toNode >= 0 && toNode < static_cast<int>(nodes.size())
        && (nodes[static_cast<std::size_t>(toNode)].kind == NodeKind::And
            || nodes[static_cast<std::size_t>(toNode)].kind == NodeKind::Or
            || nodes[static_cast<std::size_t>(toNode)].kind == NodeKind::Result);
    if (!fanIn) {
        disconnectInlet(toNode, toPort);
    }
    connections.push_back(Connection{fromNode, fromPort, toNode, toPort});
}

void NodeGraph::disconnect(int fromNode, int fromPort, int toNode, int toPort)
{
    connections.erase(std::remove_if(connections.begin(), connections.end(), [&](const Connection& c) {
        return c.fromNode == fromNode && c.fromPort == fromPort && c.toNode == toNode && c.toPort == toPort;
    }), connections.end());
}

void NodeGraph::disconnectInlet(int toNode, int toPort)
{
    connections.erase(std::remove_if(connections.begin(), connections.end(), [&](const Connection& c) {
        return c.toNode == toNode && c.toPort == toPort;
    }), connections.end());
}

int NodeGraph::findResultNode() const
{
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].kind == NodeKind::Result) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void NodeGraph::clear()
{
    nodes.clear();
    connections.clear();
}

void NodeGraph::collectReachableMetrics(int node, std::vector<std::uint8_t>& seen, std::uint32_t& passes) const
{
    if (node < 0 || node >= static_cast<int>(nodes.size()) || seen[static_cast<std::size_t>(node)]) {
        return;
    }
    seen[static_cast<std::size_t>(node)] = 1;
    const Node& n = nodes[static_cast<std::size_t>(node)];
    if (n.kind == NodeKind::Metric) {
        if (const metric::MetricDef* def = metric::metricById(n.metricId)) {
            passes |= def->requiredPasses;
        }
    }
    for (const Connection& c : connections) {
        if (c.toNode == node) {
            collectReachableMetrics(c.fromNode, seen, passes);
        }
    }
}

std::uint32_t NodeGraph::requiredPasses() const
{
    const int result = findResultNode();
    if (result < 0) {
        return 0;
    }
    std::vector<std::uint8_t> seen(nodes.size(), 0);
    std::uint32_t passes = 0;
    collectReachableMetrics(result, seen, passes);
    return passes;
}

NodeGraph::EvalVal NodeGraph::evalNode(int index, const ProbeResult& r, std::uint32_t available,
    std::vector<EvalVal>& cache, std::vector<std::uint8_t>& state) const
{
    if (index < 0 || index >= static_cast<int>(nodes.size())) {
        return {0.0, true};
    }
    const std::size_t i = static_cast<std::size_t>(index);
    if (state[i] == 2) {
        return cache[i];
    }
    if (state[i] == 1) {
        return {0.0, true}; // cycle guard
    }
    state[i] = 1;

    const Node& n = nodes[i];
    EvalVal out{0.0, true};

    auto inputs = [&](int port, std::vector<EvalVal>& dst) {
        for (const Connection& c : connections) {
            if (c.toNode == index && c.toPort == port) {
                dst.push_back(evalNode(c.fromNode, r, available, cache, state));
            }
        }
    };
    auto firstInput = [&](int port) -> EvalVal {
        for (const Connection& c : connections) {
            if (c.toNode == index && c.toPort == port) {
                return evalNode(c.fromNode, r, available, cache, state);
            }
        }
        return {0.0, true};
    };

    switch (n.kind) {
    case NodeKind::Metric: {
        const metric::MetricDef* def = metric::metricById(n.metricId);
        if (def == nullptr) {
            out = {0.0, false};
        } else if ((def->requiredPasses & ~available) != 0) {
            out = {0.0, false}; // pass not computed yet
        } else {
            metric::MetricArgs args{n.biomeArg, n.argX, n.argZ};
            out = {metric::evalMetric(n.metricId, args, r), true};
        }
        break;
    }
    case NodeKind::Constant:
        out = {n.constant, true};
        break;
    case NodeKind::Compare: {
        const EvalVal in = firstInput(0);
        if (!in.known) {
            out = {0.0, false};
        } else {
            out = {applyCompare(n.op, in.v, n.constant) ? 1.0 : 0.0, true};
        }
        break;
    }
    case NodeKind::And: {
        std::vector<EvalVal> ins;
        inputs(0, ins);
        bool allKnown = true;
        for (const EvalVal& e : ins) {
            if (e.known && e.v == 0.0) {
                out = {0.0, true}; // definitely false
                allKnown = false;  // marker handled below
                cache[i] = out;
                state[i] = 2;
                return out;
            }
            if (!e.known) {
                allKnown = false;
            }
        }
        out = {1.0, allKnown};
        break;
    }
    case NodeKind::Or: {
        std::vector<EvalVal> ins;
        inputs(0, ins);
        bool allKnownFalse = true;
        for (const EvalVal& e : ins) {
            if (e.known && e.v != 0.0) {
                out = {1.0, true};
                cache[i] = out;
                state[i] = 2;
                return out;
            }
            if (!e.known) {
                allKnownFalse = false;
            }
        }
        out = allKnownFalse ? EvalVal{0.0, true} : EvalVal{0.0, false};
        break;
    }
    case NodeKind::Not: {
        const EvalVal in = firstInput(0);
        out = in.known ? EvalVal{in.v == 0.0 ? 1.0 : 0.0, true} : EvalVal{0.0, false};
        break;
    }
    case NodeKind::Objective: {
        const EvalVal in = firstInput(0);
        if (!in.known) {
            out = {0.0, true}; // unknown contributes nothing
            break;
        }
        double span = n.normMax - n.normMin;
        double t = span != 0.0 ? (in.v - n.normMin) / span : 0.0;
        t = std::clamp(t, 0.0, 1.0);
        if (n.objectiveDir == ObjectiveDir::Minimize) {
            t = 1.0 - t;
        }
        out = {static_cast<double>(n.weight) * t, true};
        break;
    }
    case NodeKind::Result:
        out = {0.0, true};
        break;
    }

    cache[i] = out;
    state[i] = 2;
    return out;
}

GraphVerdict NodeGraph::evaluate(const ProbeResult& r, std::uint32_t availablePasses) const
{
    const int result = findResultNode();
    if (result < 0) {
        return {false, 0.0f};
    }
    std::vector<EvalVal> cache(nodes.size());
    std::vector<std::uint8_t> state(nodes.size(), 0);

    bool hardKnownFalse = false;
    double score = 0.0;
    for (const Connection& c : connections) {
        if (c.toNode != result) {
            continue;
        }
        const EvalVal e = evalNode(c.fromNode, r, availablePasses, cache, state);
        if (c.toPort == kResultHardPort) {
            if (e.known && e.v == 0.0) {
                hardKnownFalse = true;
            }
        } else if (c.toPort == kResultObjectivePort) {
            if (e.known) {
                score += e.v;
            }
        }
    }

    if (hardKnownFalse) {
        return {false, 0.0f};
    }
    return {true, static_cast<float>(1.0 + score)};
}

std::string NodeGraph::serialize() const
{
    std::ostringstream out;
    out << "seedgraph 1\n";
    for (const Node& n : nodes) {
        out << "N " << static_cast<int>(n.kind)
            << ' ' << n.metricId
            << ' ' << static_cast<int>(n.biomeArg)
            << ' ' << n.argX
            << ' ' << n.argZ
            << ' ' << n.constant
            << ' ' << static_cast<int>(n.op)
            << ' ' << static_cast<int>(n.objectiveDir)
            << ' ' << n.weight
            << ' ' << n.normMin
            << ' ' << n.normMax
            << ' ' << n.posX
            << ' ' << n.posY
            << '\n';
    }
    for (const Connection& c : connections) {
        out << "C " << c.fromNode << ' ' << c.fromPort << ' ' << c.toNode << ' ' << c.toPort << '\n';
    }
    return out.str();
}

bool NodeGraph::deserialize(const std::string& blob)
{
    clear();
    std::istringstream in(blob);
    std::string tag;
    if (!(in >> tag) || tag != "seedgraph") {
        return false;
    }
    int version = 0;
    in >> version;
    std::string line;
    while (in >> tag) {
        if (tag == "N") {
            Node n;
            int kind = 0;
            int op = 0;
            int dir = 0;
            int biome = 0;
            in >> kind >> n.metricId >> biome >> n.argX >> n.argZ >> n.constant
                >> op >> dir >> n.weight >> n.normMin >> n.normMax >> n.posX >> n.posY;
            n.kind = static_cast<NodeKind>(kind);
            n.biomeArg = static_cast<std::uint8_t>(biome);
            n.op = static_cast<CompareOp>(op);
            n.objectiveDir = static_cast<ObjectiveDir>(dir);
            nodes.push_back(n);
        } else if (tag == "C") {
            Connection c;
            in >> c.fromNode >> c.fromPort >> c.toNode >> c.toPort;
            connections.push_back(c);
        } else {
            std::getline(in, line); // skip unknown line
        }
    }
    return true;
}

} // namespace seedfinder::graph
