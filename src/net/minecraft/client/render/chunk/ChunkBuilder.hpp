#pragma once

#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/client/render/chunk/ChunkDrawTransform.hpp"
#include "net/minecraft/client/render/culling/Culler.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/World.hpp"

#include <array>
#include <cmath>
#include <vector>

namespace net::minecraft::client::render::chunk {

struct ChunkMeshJob;

class ChunkBuilder {
public:
    ChunkBuilder(World* world, std::vector<::net::minecraft::block::entity::BlockEntity*>& blockEntityUpdateList, int x,
        int y, int z, int size, int baseRenderListId)
        : world(world),
          baseRenderList(baseRenderListId),
          x(x),
          y(y),
          z(z),
          currentBlockEntities_(&blockEntityUpdateList)
    {
        sizeX = size;
        sizeY = size;
        sizeZ = size;
        radius = std::sqrt(static_cast<float>(sizeX * sizeX + sizeY * sizeY + sizeZ * sizeZ)) / 2.0f;
        centerX = this->x + sizeX / 2;
        centerY = this->y + sizeY / 2;
        centerZ = this->z + sizeZ / 2;
        renderX = this->x & 0x3FF;
        renderY = this->y;
        renderZ = this->z & 0x3FF;
        cameraOffsetX = this->x - renderX;
        cameraOffsetY = this->y - renderY;
        cameraOffsetZ = this->z - renderZ;
        constexpr float padding = 6.0f;
        cullingBox = net::minecraft::Box(
            static_cast<double>(this->x) - padding,
            static_cast<double>(this->y) - padding,
            static_cast<double>(this->z) - padding,
            static_cast<double>(this->x + sizeX) + padding,
            static_cast<double>(this->y + sizeY) + padding,
            static_cast<double>(this->z + sizeZ) + padding);
        dirty = false;
    }

    [[nodiscard]] float squaredDistanceTo(double entityX, double entityY, double entityZ) const
    {
        const float dx = static_cast<float>(entityX - static_cast<double>(centerX));
        const float dy = static_cast<float>(entityY - static_cast<double>(centerY));
        const float dz = static_cast<float>(entityZ - static_cast<double>(centerZ));
        return dx * dx + dy * dy + dz * dz;
    }

    // Worker-thread half: tessellate the job's snapshot into CPU meshes.
    // Touches no GL and no live world state.
    static void buildMesh(ChunkMeshJob& job);

    // Main-thread half: compile the captured meshes into this builder's GL
    // display lists, resolve block entities, and publish flags.
    void uploadMesh(const ChunkMeshJob& job);

    void updateFrustum(const Culler& culler)
    {
        inFrustum = culler.isVisible(cullingBox);
    }

    [[nodiscard]] bool hasNoGeometry() const noexcept
    {
        return built && renderLayerEmpty[0] && renderLayerEmpty[1];
    }

    void invalidate() noexcept
    {
        dirty = true;
        ++version;
    }

    World* world = nullptr;
    int baseRenderList = -1;
    inline static int chunkUpdates = 0;
    int x = 0;
    int y = 0;
    int z = 0;
    int sizeX = 0;
    int sizeY = 0;
    int sizeZ = 0;
    int cameraOffsetX = 0;
    int cameraOffsetY = 0;
    int cameraOffsetZ = 0;
    int renderX = 0;
    int renderY = 0;
    int renderZ = 0;
    bool inFrustum = false;
    std::array<bool, 2> renderLayerEmpty {true, true};
    int centerX = 0;
    int centerY = 0;
    int centerZ = 0;
    float radius = 0.0f;
    bool dirty = false;
    net::minecraft::Box cullingBox {0, 0, 0, 0, 0, 0};
    int id = 0;
    bool hasSkyLight = false;
    bool built = false;
    // Bumped on every invalidation so stale captures are ignored.
    int version = 0;
    // A mesh job for this builder is queued or running (main-thread bookkeeping).
    bool meshJobInFlight = false;
    // Section was evicted while a mesh job was still in flight; the result is
    // dropped and the display-list pair is recycled once the job completes.
    bool retired = false;
    std::vector<::net::minecraft::block::entity::BlockEntity*> blockEntities_ {};
    std::vector<::net::minecraft::block::entity::BlockEntity*>* currentBlockEntities_ = nullptr;
};

} // namespace net::minecraft::client::render::chunk
