#pragma once
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/chunk/ChunkMeshScheduler.hpp"
#include "net/minecraft/client/render/chunk/ChunkRegionManager.hpp"
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
class WorldRenderer : public net::minecraft::GameEventListener {
public:
  WorldRenderer(net::minecraft::client::Minecraft* minecraft = nullptr,
                net::minecraft::client::texture::TextureManager* textureManager = nullptr);
  void setWorld(net::minecraft::World* world);
  void reload();
  void reloadIfViewDistanceChanged();
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
  void releaseSections();
  std::vector<net::minecraft::block::entity::BlockEntity*> globalBlockEntities{};
  float miningProgress = 0.0f;
  net::minecraft::client::Minecraft* client = nullptr;
  net::minecraft::World* world = nullptr;
  net::minecraft::client::texture::TextureManager* textureManager = nullptr;
  net::minecraft::client::render::block::BlockRenderManager blockRenderManager{};
  void setCamera(net::minecraft::Entity* camera) {
    cameraEntity_ = camera;
  }
  void setOptions(net::minecraft::client::option::GameOptions* options) {
    options_ = options;
  }

private:
  void renderOutline(const net::minecraft::Box& box);
  [[nodiscard]] net::minecraft::client::option::GameOptions& activeOptions() const;
  [[nodiscard]] const net::minecraft::LivingEntity* frontierCamera() const;
  [[nodiscard]] chunk::ChunkBuilder* sectionAt(int sectionX, int sectionY, int sectionZ);
  void enqueueDirtyChunk(chunk::ChunkBuilder* chunk);
  void noteNearDirty(chunk::ChunkBuilder* chunk);
  bool startMeshJob(chunk::ChunkBuilder* chunk, bool nearLane, int priority,
                    const client::option::ResolvedRenderOptions& resolvedOpts, bool fancyGraphics);
  void renderChunks(int layer, double tickDelta);
  int renderChunksVbo(int layer, double tickDelta, double interpX, double interpY, double interpZ);
  void renderModChunkMeshes(int layer, double interpX, double interpY, double interpZ);
  void updateSectionFrontier();
  void drainPendingColumns();
  void createColumn(int sectionX, int sectionZ);
  void enqueueColumn(int sectionX, int sectionZ);
  void retireOrFreeSection(std::unique_ptr<chunk::ChunkBuilder> section);
  void sweepRetiring();
  void rebuildDrawRings();
  void rebuildVisibleDrawRings();
  void clearSections();
  [[nodiscard]] int ringOf(int sectionX, int sectionZ) const noexcept;
  [[nodiscard]] int ringOf(const chunk::ChunkBuilder& chunk) const noexcept;
  std::unordered_map<world::SectionPos, std::unique_ptr<chunk::ChunkBuilder>, world::SectionPosHash> sections_{};
  std::unordered_set<chunk::ChunkBuilder*> dirtyChunks_{};
  std::unordered_set<chunk::ChunkBuilder*> nearDirtyChunks_{};
  std::vector<std::vector<chunk::ChunkBuilder*>> drawRings_{};
  std::vector<std::vector<chunk::ChunkBuilder*>> visibleDrawRings_{};
  std::deque<world::SectionPos> pendingColumns_{};
  std::unordered_set<world::SectionPos, world::SectionPosHash> pendingSet_{};
  std::vector<std::unique_ptr<chunk::ChunkBuilder>> retiring_{};
  chunk::ChunkRegionManager regionManager_{};
  chunk::ChunkMeshScheduler meshScheduler_{};
  std::vector<std::shared_ptr<chunk::ChunkMeshJob>> pendingMeshUploads_{};
  int centerSectionX_ = std::numeric_limits<int>::min();
  int centerSectionZ_ = std::numeric_limits<int>::min();
  int renderRadiusChunks_ = 0;
  int nextSectionId_ = 0;
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
  int lastDrawnRegionCount_ = 0;
  net::minecraft::Entity* cameraEntity_ = nullptr;
  net::minecraft::client::option::GameOptions* options_ = nullptr;
  net::minecraft::client::option::GameOptions defaultOptions_{};
};
} // namespace net::minecraft::client::render
