#pragma once



#include "net/minecraft/client/option/GameOptions.hpp"

#include "net/minecraft/client/render/block/BlockRenderManager.hpp"

#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/chunk/ChunkMeshScheduler.hpp"
#include "net/minecraft/client/render/world/AsyncChunkSorter.hpp"
#include "net/minecraft/client/render/world/ChunkRenderer.hpp"

#include "net/minecraft/entity/EntityTypes.hpp"

#include "net/minecraft/item/ItemStack.hpp"

#include "net/minecraft/util/hit/HitResult.hpp"

#include "net/minecraft/util/math/Types.hpp"

#include "net/minecraft/world/events/GameEventListener.hpp"



#include <array>

#include <cstdint>

#include <limits>

#include <memory>

#include <string>

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

// Native world renderer inspired by beta 1.7.3, with conservative CPU-side

// culling and deterministic chunk scheduling.

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



    void sortChunks(int cameraX, int cameraY, int cameraZ);

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



    void playStreaming(const std::string& stream, int x, int y, int z) override;

    void playSound(const std::string& sound, double x, double y, double z, float volume, float pitch) override;

    void addParticle(const std::string& particle, double x, double y, double z, double velocityX, double velocityY,

        double velocityZ) override;

    void notifyEntityAdded(net::minecraft::Entity* entity) override;

    void notifyEntityRemoved(net::minecraft::Entity* entity) override;

    void notifyAmbientDarknessChanged() override;

    void updateBlockEntity(int x, int y, int z, net::minecraft::block::entity::BlockEntity* blockEntity) override;

    void onEntityPickup(net::minecraft::Entity* entity, net::minecraft::PlayerEntity* collector) override;

    void worldEvent(net::minecraft::PlayerEntity* player, int type, int x, int y, int z, int data) override;



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



    [[nodiscard]] std::size_t chunkIndex(int chunkX, int chunkY, int chunkZ) const;

    chunk::ChunkBuilder* chunkAt(int chunkX, int chunkY, int chunkZ);

    void enqueueDirtyChunk(chunk::ChunkBuilder* chunk);

    // Record chunks dirtied within kNearDirtyDistSq of the camera so
    // compileChunks can route them to the dedicated near mesh worker.
    void noteNearDirty(chunk::ChunkBuilder* chunk);

    void sortChunksOnMove(const net::minecraft::LivingEntity& camera);

    // Hand the current chunk centers to the background sorter (keys only, no
    // pointers) and apply its previous result if one is ready.
    void submitChunkSort(double cameraX, double cameraY, double cameraZ);
    void pollChunkSort();

    void expectFramePhase(FramePhase required) const;
    void advanceFramePhase(FramePhase next) noexcept;

    FramePhase framePhase_ = FramePhase::Idle;

    std::vector<net::minecraft::client::render::chunk::ChunkBuilder> chunks_ {};

    std::unordered_set<net::minecraft::client::render::chunk::ChunkBuilder*> dirtyChunks_ {};

    // Fast lane: dirty chunks near the camera (block edits). Entries also stay
    // in dirtyChunks_ until a mesh job is enqueued; stale entries are dropped
    // by compileChunks' dirty/in-flight re-check. Cleared with dirtyChunks_.
    std::vector<net::minecraft::client::render::chunk::ChunkBuilder*> nearDirtyChunks_ {};

    // Declared after chunks_: destroyed first, so the mesh worker pool drains
    // before the ChunkBuilders its jobs point at are destroyed.
    chunk::ChunkMeshScheduler meshScheduler_ {};
    std::vector<std::shared_ptr<chunk::ChunkMeshJob>> pendingMeshUploads_ {};

    std::vector<net::minecraft::client::render::chunk::ChunkBuilder*> sortedChunks_ {};

    // Background distance sorter. It only ever holds (key, index) pairs, so it
    // has no destruction-order relationship with chunks_; chunkArrayEpoch_ is
    // bumped whenever chunks_ is rebuilt so stale results are discarded.
    world::AsyncChunkSorter chunkSorter_ {};
    // Separate instance prioritizing the dirty backlog for compileChunks, so
    // a long draw-order sort can't starve mesh scheduling (and vice versa).
    world::AsyncChunkSorter dirtySorter_ {};
    std::uint64_t chunkArrayEpoch_ = 0;

    std::vector<net::minecraft::client::render::chunk::ChunkBuilder*> chunksInCurrentLayer_ {};

    std::vector<world::ChunkRenderer> chunkRenderers_ {};

    int chunkCountX = 0;

    int chunkCountY = 0;

    int chunkCountZ = 0;

    int chunkGlList = 0;

    int minChunkX = 0;

    int minChunkY = 0;

    int minChunkZ = 0;

    int maxChunkX = 0;

    int maxChunkY = 0;

    int maxChunkZ = 0;

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

    int chunkRenderIndex = 0;

    int cullStep = 0;

    double prevCameraX = -9999.0;

    double prevCameraY = -9999.0;

    double prevCameraZ = -9999.0;



    Culler* lastCuller_ = nullptr;

    net::minecraft::Entity* cameraEntity_ = nullptr;

    net::minecraft::client::option::GameOptions* options_ = nullptr;

    net::minecraft::client::option::GameOptions defaultOptions_ {};

};



} // namespace net::minecraft::client::render
