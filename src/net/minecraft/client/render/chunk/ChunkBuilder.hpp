#pragma once

#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/client/render/chunk/ChunkDrawTransform.hpp"
#include "net/minecraft/client/render/culling/Culler.hpp"
#include "net/minecraft/entity/Entity.hpp"
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
          currentBlockEntities_(&blockEntityUpdateList)
    {
        sizeX = size;
        sizeY = size;
        sizeZ = size;
        radius = std::sqrt(static_cast<float>(sizeX * sizeX + sizeY * sizeY + sizeZ * sizeZ)) / 2.0f;
        this->x = -999;
        setPosition(x, y, z);
        dirty = false;
    }

    void setPosition(int newX, int newY, int newZ);

    [[nodiscard]] float squaredDistanceTo(double entityX, double entityY, double entityZ) const
    {
        const float dx = static_cast<float>(entityX - static_cast<double>(centerX));
        const float dy = static_cast<float>(entityY - static_cast<double>(centerY));
        const float dz = static_cast<float>(entityZ - static_cast<double>(centerZ));
        return dx * dx + dy * dy + dz * dz;
    }

    [[nodiscard]] float squaredDistanceTo(const Entity& entity) const
    {
        const float dx = static_cast<float>(entity.x - static_cast<double>(centerX));
        const float dy = static_cast<float>(entity.y - static_cast<double>(centerY));
        const float dz = static_cast<float>(entity.z - static_cast<double>(centerZ));
        return dx * dx + dy * dy + dz * dz;
    }

    // Synchronous main-thread rebuild (forced compiles, fallback path).
    // Internally: snapshot -> buildMesh -> uploadMesh, same as the async path.
    void rebuild();

    // Worker-thread half: tessellate the job's snapshot into CPU meshes.
    // Touches no GL and no live world state.
    static void buildMesh(ChunkMeshJob& job);

    // Main-thread half: compile the captured meshes into this builder's GL
    // display lists, resolve block entities, and publish flags.
    void uploadMesh(const ChunkMeshJob& job);

    [[nodiscard]] bool hasLayerGeometry(int layerId) const noexcept
    {
        return inFrustum && !renderLayerEmpty[static_cast<std::size_t>(layerId)];
    }

    void renderLayer(int layerId, double interpX, double interpY, double interpZ) const;

    void reset();
    void close();

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
    bool queuedForRebuild = false;
    // Bumped on every invalidation; mesh results built from an older version
    // are discarded at upload time.
    int version = 0;
    // A mesh job for this builder is queued or running (main-thread bookkeeping).
    bool meshJobInFlight = false;
    std::vector<::net::minecraft::block::entity::BlockEntity*> blockEntities_ {};
    std::vector<::net::minecraft::block::entity::BlockEntity*>* currentBlockEntities_ = nullptr;
};

} // namespace net::minecraft::client::render::chunk
