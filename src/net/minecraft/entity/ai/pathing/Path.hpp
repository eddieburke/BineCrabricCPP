#pragma once

#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/ai/pathing/PathNode.hpp"
#include "net/minecraft/util/math/Types.hpp"

#include <vector>

namespace net::minecraft::entity::ai::pathing {

class Path {
public:
    Path() = default;

    explicit Path(std::vector<PathNode> nodesIn)
        : nodes(std::move(nodesIn))
    {
        length = static_cast<int>(nodes.size());
    }

    Path(Path&&) noexcept = default;
    Path& operator=(Path&&) noexcept = default;
    Path(const Path&) = default;
    Path& operator=(const Path&) = default;

    [[nodiscard]] bool isFinished() const { return currentNodeIndex >= length; }
    void next() { ++currentNodeIndex; }

    [[nodiscard]] const PathNode* getEnd() const
    {
        if (length <= 0) {
            return nullptr;
        }
        return &nodes[static_cast<std::size_t>(length - 1)];
    }

    [[nodiscard]] Vec3d getNodePosition(const Entity& entity) const
    {
        if (currentNodeIndex < 0 || currentNodeIndex >= length) {
            return {};
        }
        const PathNode& node = nodes[static_cast<std::size_t>(currentNodeIndex)];
        const double offset = static_cast<double>(static_cast<int>(entity.width + 1.0f)) * 0.5;
        return Vec3d {static_cast<double>(node.x) + offset, static_cast<double>(node.y),
            static_cast<double>(node.z) + offset};
    }

    int length = 0;

private:
    std::vector<PathNode> nodes;
    int currentNodeIndex = 0;
};

} // namespace net::minecraft::entity::ai::pathing
