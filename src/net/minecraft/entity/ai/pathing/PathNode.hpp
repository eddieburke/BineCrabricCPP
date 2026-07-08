#pragma once
#include "net/minecraft/util/math/MathHelper.hpp"

namespace net::minecraft::entity::ai::pathing {
class PathNode {
   public:
    int x = 0;
    int y = 0;
    int z = 0;
    PathNode() = default;

    PathNode(int xIn, int yIn, int zIn) : x(xIn), y(yIn), z(zIn), hashCode_(hash(xIn, yIn, zIn)) {
    }

    static int hash(int x, int y, int z) {
        return (y & 0xFF) | ((x & 0x7FFF) << 8) | ((z & 0x7FFF) << 24) | (x < 0 ? static_cast<int>(0x80000000U) : 0) |
               (z < 0 ? 32768 : 0);
    }

    [[nodiscard]] float getDistance(const PathNode& node) const {
        const float dx = static_cast<float>(node.x - x);
        const float dy = static_cast<float>(node.y - y);
        const float dz = static_cast<float>(node.z - z);
        return MathHelper::sqrt(dx * dx + dy * dy + dz * dz);
    }

    [[nodiscard]] bool equals(const PathNode& other) const {
        return hashCode_ == other.hashCode_ && x == other.x && y == other.y && z == other.z;
    }

    [[nodiscard]] bool isInHeap() const {
        return heapIndex >= 0;
    }

    int heapIndex = -1;
    float penalizedPathLength = 0.0f;
    float distanceToNearestTarget = 0.0f;
    float heapWeight = 0.0f;
    PathNode* previous = nullptr;
    bool visited = false;

   private:
    int hashCode_ = 0;
};
}  // namespace net::minecraft::entity::ai::pathing
