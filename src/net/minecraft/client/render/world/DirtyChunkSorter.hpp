#pragma once

#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/entity/Entity.hpp"

namespace net::minecraft::client::render::world {

class DirtyChunkSorter {
public:
    explicit DirtyChunkSorter(const Entity& camera)
        : camera_(&camera)
    {
    }

    DirtyChunkSorter(double cameraX, double cameraY, double cameraZ)
        : cameraX_(cameraX),
          cameraY_(cameraY),
          cameraZ_(cameraZ)
    {
    }

    [[nodiscard]] double distanceSquared(const chunk::ChunkBuilder& chunk) const
    {
        return camera_ != nullptr ? static_cast<double>(chunk.squaredDistanceTo(*camera_))
                                  : chunk.squaredDistanceTo(cameraX_, cameraY_, cameraZ_);
    }

    // Matches Java Comparator.compare — farther chunks sort earlier (ascending).
    [[nodiscard]] int compare(const chunk::ChunkBuilder* first, const chunk::ChunkBuilder* second) const
    {
        if (first == second) {
            return 0;
        }
        if (first == nullptr) {
            return -1;
        }
        if (second == nullptr) {
            return 1;
        }
        if (first->inFrustum && !second->inFrustum) {
            return 1;
        }
        if (second->inFrustum && !first->inFrustum) {
            return -1;
        }
        const double firstDistance = distanceSquared(*first);
        const double secondDistance = distanceSquared(*second);
        if (firstDistance < secondDistance) {
            return 1;
        }
        if (firstDistance > secondDistance) {
            return -1;
        }
        return first->id < second->id ? 1 : -1;
    }

    [[nodiscard]] bool operator()(const chunk::ChunkBuilder* first, const chunk::ChunkBuilder* second) const
    {
        return compare(first, second) < 0;
    }

private:
    const Entity* camera_ = nullptr;
    double cameraX_ = 0.0;
    double cameraY_ = 0.0;
    double cameraZ_ = 0.0;
};

} // namespace net::minecraft::client::render::world
