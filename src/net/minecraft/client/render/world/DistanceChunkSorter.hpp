#pragma once

#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/entity/Entity.hpp"

namespace net::minecraft::client::render::world {

class DistanceChunkSorter {
public:
    explicit DistanceChunkSorter(const Entity& camera)
        : cameraX_(-camera.x),
          cameraY_(-camera.y),
          cameraZ_(-camera.z)
    {
    }

    DistanceChunkSorter(double cameraX, double cameraY, double cameraZ)
        : cameraX_(-cameraX),
          cameraY_(-cameraY),
          cameraZ_(-cameraZ)
    {
    }

    [[nodiscard]] double distanceSquared(const chunk::ChunkBuilder& chunk) const
    {
        const double dx = static_cast<double>(chunk.centerX) + cameraX_;
        const double dy = static_cast<double>(chunk.centerY) + cameraY_;
        const double dz = static_cast<double>(chunk.centerZ) + cameraZ_;
        return dx * dx + dy * dy + dz * dz;
    }

    // Matches Java DistanceChunkSorter.compare — scaled distance delta as sort key.
    [[nodiscard]] int compare(const chunk::ChunkBuilder* first, const chunk::ChunkBuilder* second) const
    {
        if (first == second) {
            return 0;
        }
        if (first == nullptr) {
            return 0;
        }
        if (second == nullptr) {
            return 0;
        }
        const double firstDistance = distanceSquared(*first);
        const double secondDistance = distanceSquared(*second);
        return static_cast<int>((firstDistance - secondDistance) * 1024.0);
    }

    [[nodiscard]] bool operator()(const chunk::ChunkBuilder* first, const chunk::ChunkBuilder* second) const
    {
        return compare(first, second) < 0;
    }

private:
    double cameraX_ = 0.0;
    double cameraY_ = 0.0;
    double cameraZ_ = 0.0;
};

} // namespace net::minecraft::client::render::world
