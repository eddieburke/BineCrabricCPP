#pragma once

#include "net/minecraft/client/option/GameOptions.hpp"

#include "net/minecraft/client/render/block/BlockRenderManager.hpp"

#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/chunk/ChunkMeshScheduler.hpp"
#include "net/minecraft/client/render/chunk/GlListPool.hpp"
#include "net/minecraft/client/render/world/ChunkRenderer.hpp"
#include "net/minecraft/client/render/world/SectionPos.hpp"

#include "net/minecraft/item/ItemStack.hpp"

#include "net/minecraft/util/hit/HitResult.hpp"

#include "net/minecraft/util/math/Types.hpp"

#include "net/minecraft/world/events/GameEventListener.hpp"

#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace net::minecraft::block::entity {
class BlockEntity;
}

namespace net::minecraft {
class World;
}

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft::client::texture {
class TextureManager;
}

namespace net::minecraft::client::render {

class Culler;

namespace internal {
class WorldRendererCore;
}

// Native world renderer. Chunk geometry is stored in a sparse, camera-centered
// map of 16^3 render sections (no toroidal fixed grid): sections are allocated
// as the camera nears their column and freed as it leaves, so render distance is
// limited only by memory and there are no wraparound artifacts or fixed
// display-list ceiling. Visibility, meshing, and draw order are all driven from
// this section set.
class WorldRenderer : public net::minecraft::GameEventListener {

    friend class internal::WorldRendererCore;

public:
    WorldRenderer(net::minecraft::client::Minecraft* minecraft = nullptr,
        net::minecraft::client::texture::TextureManager* textureManager = nullptr);

    void setWorld(net::minecraft::World* world);

    void reload();

    void reloadIfViewDistanceChanged();

    void beginWorldRenderFrame() noexcept;

    void renderEntities(const Vec3d& cameraPos, Culler* culler, float tickDelta);

    [[nodiscard]] std::string getChunkDebugInfo() const;

    [[nodiscard]] std::string getEntityDebugInfo() const;

    int render(net::minecraft::LivingEntity& camera, int layer, double tickDelta);

    void render(const net::minecraft::Entity& camera, int layer, float tickDelta);

    bool compileChunks(net::minecraft::LivingEntity& camera, bool force);

    void renderLastChunks(int layer, double tickDelta);

    void renderMiningProgress(net::minecraft::PlayerEntity* entity, const net::minecraft::HitResult& hit, int i,
        const net::minecraft::ItemStack& handStack, float tickDelta);

    void renderBlockOutline(net::minecraft::PlayerEntity* player, const net::minecraft::HitResult& hitResult, int i,
        const net::minecraft::ItemStack& handStack, float tickDelta);

    void markDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ);

    void blockUpdate(int x, int y, int z) override;

    void setBlocksDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) override;

    void cullChunks(Culler* culler, float tickDelta);

    void addParticle(const std::string& particle, double x, double y, double z, double velocityX, double velocityY,
        double velocityZ) override;

    void notifyEntityAdded(net::minecraft::Entity* entity) override;

    void notifyEntityRemoved(net::minecraft::Entity* entity) override;

    void notifyAmbientDarknessChanged() override;

    void updateBlockEntity(int x, int y, int z, net::minecraft::block::entity::BlockEntity* blockEntity) override;

    void onEntityPickup(net::minecraft::Entity* entity, net::minecraft::PlayerEntity* collector) override;

    void blockBreakParticles(int x, int y, int z, int blockId, int blockMeta) override;

    void releaseGlLists();

    // Java public fields.
    std::vector<net::minecraft::block::entity::BlockEntity*> globalBlockEntities {};

    float miningProgress = 0.0f;

    net::minecraft::client::Minecraft* client = nullptr;

    net::minecraft::World* world = nullptr;

    net::minecraft::client::texture::TextureManager* textureManager = nullptr;

    net::minecraft::client::render::block::BlockRenderManager blockRenderManager {};

    // Native integration helpers (not in Java).
    void setCamera(net::minecraft::Entity* camera) { cameraEntity_ = camera; }

    void setOptions(net::minecraft::client::option::GameOptions* options) { options_ = options; }

private:
    enum class FramePhase : std::uint8_t {
        Idle = 0,
        Culled,
        Compiled,
        SolidLayerDone,
    };

    void renderOutline(const net::minecraft::Box& box);

    [[nodiscard]] net::minecraft::client::option::GameOptions& activeOptions() const;

    [[nodiscard]] const net::minecraft::LivingEntity* resolveSortCamera() const;

    [[nodiscard]] chunk::ChunkBuilder* sectionAt(int sectionX, int sectionY, int sectionZ);

    void enqueueDirtyChunk(chunk::ChunkBuilder* chunk);

    // Record a section dirtied within reach of the camera (a block edit) so
    // compileChunks routes it to the dedicated near mesh worker for next-frame
    // refresh, regardless of how large the distant backlog is.
    void noteNearDirty(chunk::ChunkBuilder* chunk);

    // Sparse-section lifecycle. updateSectionFrontier recenters the section set
    // on the camera and diffs in/out the rings that changed; drainPendingColumns
    // creates sections (budgeted) for in-range columns once their world chunk is
    // resident; the rest free / retire sections and maintain the near->far draw
    // ring buckets.
    void updateSectionFrontier();
    void drainPendingColumns();
    void createColumn(int sectionX, int sectionZ);
    void enqueueColumn(int sectionX, int sectionZ);
    void retireOrFreeSection(std::unique_ptr<chunk::ChunkBuilder> section);
    void sweepRetiring();
    void rebuildDrawRings();
    void clearSections();
    [[nodiscard]] int ringOf(int sectionX, int sectionZ) const noexcept;

    void expectFramePhase(FramePhase required) const;
    void advanceFramePhase(FramePhase next) noexcept;

    FramePhase framePhase_ = FramePhase::Idle;

    // Sparse render-section store. unique_ptr keeps ChunkBuilder addresses stable
    // across rehash, so dirtyChunks_/drawRings_/mesh jobs can hold raw pointers.
    std::unordered_map<world::SectionPos, std::unique_ptr<chunk::ChunkBuilder>, world::SectionPosHash> sections_ {};

    std::unordered_set<chunk::ChunkBuilder*> dirtyChunks_ {};

    // Fast lane: sections dirtied next to the camera (block edits). Also kept in
    // dirtyChunks_ until enqueued; entries are dropped when a section is freed
    // (retireOrFreeSection) so no dangling pointers survive.
    std::vector<chunk::ChunkBuilder*> nearDirtyChunks_ {};

    // Near->far draw order: drawRings_[r] holds the sections at Chebyshev ring r
    // from the camera section. Rebuilt on recenter; appended on lazy create.
    std::vector<std::vector<chunk::ChunkBuilder*>> drawRings_ {};

    // Columns (section x/z, y unused) in range but not yet backed by a section,
    // awaiting world-chunk residency. Drained round-robin under a per-frame budget.
    std::deque<world::SectionPos> pendingColumns_ {};
    std::unordered_set<world::SectionPos, world::SectionPosHash> pendingSet_ {};

    // Sections evicted while a mesh job was still in flight for them: kept alive
    // (so the worker's ChunkBuilder* stays valid) until the job completes, then
    // their display-list pair is recycled. See sweepRetiring.
    std::vector<std::unique_ptr<chunk::ChunkBuilder>> retiring_ {};

    chunk::GlListPool listPool_ {};

    // Declared after the section containers so the mesh worker pool drains before
    // the ChunkBuilders its jobs point at are destroyed.
    chunk::ChunkMeshScheduler meshScheduler_ {};
    std::vector<std::shared_ptr<chunk::ChunkMeshJob>> pendingMeshUploads_ {};

    std::vector<chunk::ChunkBuilder*> chunksInCurrentLayer_ {};

    std::vector<world::ChunkRenderer> chunkRenderers_ {};

    int centerSectionX_ = std::numeric_limits<int>::min();
    int centerSectionZ_ = std::numeric_limits<int>::min();
    int renderRadiusChunks_ = 0;
    int nextSectionId_ = 0;
    int cullStep = 0;

    int lastViewDistance = -1;
    float lastRenderScale = -1.0f;

    int entityRenderCooldown = 2;

    int entityCount = 0;

    int renderedEntityCount = 0;

    int culledEntityCount = 0;

    int chunkCount = 0;

    int invisibleChunkCount = 0;

    int compiledChunkCount = 0;

    int emptyChunkCount = 0;

    net::minecraft::Entity* cameraEntity_ = nullptr;

    net::minecraft::client::option::GameOptions* options_ = nullptr;

    net::minecraft::client::option::GameOptions defaultOptions_ {};
};

} // namespace net::minecraft::client::render
