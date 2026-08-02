#pragma once
#include <array>
#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/util/hit/HitResult.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
#include "net/minecraft/util/math/MatrixStack.hpp"
#include "net/minecraft/world/events/GameEventListener.hpp"
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
namespace world {
struct SectionPos {
 int x = 0;
 int y = 0;
 int z = 0;
 [[nodiscard]] bool operator==(const SectionPos& other) const noexcept {
  return x == other.x && y == other.y && z == other.z;
 }
};
struct SectionPosHash {
 [[nodiscard]] std::size_t operator()(const SectionPos& p) const noexcept {
  std::size_t h = static_cast<std::uint32_t>(p.x) * 0x9E3779B1u;
  h ^= static_cast<std::uint32_t>(p.z) * 0x85EBCA77u + (h << 6) + (h >> 2);
  h ^= static_cast<std::uint32_t>(p.y) * 0xC2B2AE3Du + (h << 6) + (h >> 2);
  return h;
 }
};
// P-LITGATE: chunk columns whose first mesh is held until their lighting
// drains (World::doLightingUpdates marks columns lit per drained region, and
// releases everything once the engine goes fully idle — non-optional).
struct ColumnLightingGate {
 [[nodiscard]] bool lit(int sectionX, int sectionZ) const noexcept {
  return pending_.find(SectionPos{sectionX, 0, sectionZ}) == pending_.end();
 }
 void gate(int sectionX, int sectionZ) {
  pending_.insert(SectionPos{sectionX, 0, sectionZ});
 }
 void release(int sectionX, int sectionZ) {
  pending_.erase(SectionPos{sectionX, 0, sectionZ});
 }
 void releaseAll() {
  pending_.clear();
 }
 [[nodiscard]] bool empty() const noexcept {
  return pending_.empty();
 }
 std::unordered_set<SectionPos, SectionPosHash> pending_{};
};
} // namespace world
class FrustumCuller;
class WorldRenderer : public net::minecraft::GameEventListener {
 friend class GameRenderer;

 public:
 WorldRenderer(net::minecraft::client::Minecraft* minecraft = nullptr,
               net::minecraft::client::texture::TextureManager* textureManager = nullptr);
 void setWorld(net::minecraft::World* world);
 void reload();
 void reloadIfViewDistanceChanged();
  // Entities compose their poses onto the iris per-draw base (core::drawModelView)
  // instead of a separately-maintained camera stack; the projection is the iris draw
  // projection. cameraPos is the frame camera position (RenderCameraState).
  void renderEntities(const Vec3d& cameraPos, FrustumCuller* culler, float tickDelta);
 [[nodiscard]] std::string getChunkDebugInfo() const;
 [[nodiscard]] std::string getEntityDebugInfo() const;
 int render(net::minecraft::LivingEntity& camera, int layer, double tickDelta, bool drawModMeshes = true);
 bool compileChunks(net::minecraft::LivingEntity& camera, bool force);
 void renderLastChunks(int layer, double tickDelta);
 void renderMiningProgress(net::minecraft::PlayerEntity* entity,
                           const net::minecraft::HitResult& hit,
                           int i,
                           const net::minecraft::ItemStack& handStack,
                           float tickDelta);
 void renderBlockOutline(net::minecraft::PlayerEntity* player,
                         const net::minecraft::HitResult& hitResult,
                         int i,
                         const net::minecraft::ItemStack& handStack,
                         float tickDelta);
 void markDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ);
 void blockUpdate(int x, int y, int z) override;
 void setBlocksDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) override;
   void chunkAvailable(int chunkX, int chunkZ) override;
   void chunkUnloaded(int chunkX, int chunkZ) override;
   void markChunkColumnLit(int chunkX, int chunkZ) override;
  void markAllChunksLit() override;
 void cullChunks(FrustumCuller* culler, float tickDelta, bool updateFrontier = true);
 void addParticle(const std::string& particle,
                  double x,
                  double y,
                  double z,
                  double velocityX,
                  double velocityY,
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
 void setRenderCameraEntity(bool renderCameraEntity) noexcept {
  renderCameraEntity_ = renderCameraEntity;
 }
 // Force section columns to rebuild around the next camera position (e.g. first
 // server teleport after the loading screen tracked a stale position).
 void resetSectionFrontier() noexcept {
  centerSectionX_ = std::numeric_limits<int>::min();
  centerSectionZ_ = std::numeric_limits<int>::min();
 }
 void setOptions(net::minecraft::client::option::GameOptions* options) {
  options_ = options;
  settings_ = option::renderSettings(activeOptions());
 }
 void setRenderSettings(const net::minecraft::client::option::RenderSettings& settings) { settings_ = settings; }
 void pushCullState();
 void popCullState();

 private:
 void renderOutline(const net::minecraft::Box& box);
 [[nodiscard]] net::minecraft::client::option::GameOptions& activeOptions() const;
 [[nodiscard]] const net::minecraft::LivingEntity* frontierCamera() const;
 [[nodiscard]] chunk::ChunkBuilder* sectionAt(int sectionX, int sectionY, int sectionZ);
 void enqueueDirtyChunk(chunk::ChunkBuilder* chunk);
 void noteNearDirty(chunk::ChunkBuilder* chunk);
 bool startMeshJob(chunk::ChunkBuilder* chunk,
                   bool nearLane,
                   int priority,
                   const client::option::RenderSettings& resolvedOpts,
                   bool fancyGraphics);
 void renderChunks(int layer, double tickDelta, bool drawModMeshes = true, bool skipBuildDrawLists = false);
 int renderChunksVbo(
     int layer, double tickDelta, double interpX, double interpY, double interpZ, bool skipBuildDrawLists = false);
 int renderModChunkMeshes(int layer, double interpX, double interpY, double interpZ);
 void updateSectionFrontier();
 void drainPendingColumns();
 void createColumn(int sectionX, int sectionZ);
 void removeColumn(int sectionX, int sectionZ);
 void enqueueColumn(int sectionX, int sectionZ);
 void retireOrFreeSection(std::unique_ptr<chunk::ChunkBuilder> section);
 void sweepRetiring();
 void rebuildDrawRings();
 void rebuildVisibleDrawRings();
 void applyOcclusionCulling();
 void clearSections();
 [[nodiscard]] int ringOf(int sectionX, int sectionZ) const noexcept;
 [[nodiscard]] int ringOf(const chunk::ChunkBuilder& chunk) const noexcept;
  std::unordered_map<world::SectionPos, std::unique_ptr<chunk::ChunkBuilder>, world::SectionPosHash> sections_{};
  std::vector<chunk::ChunkBuilder*> sectionList_{};
  // P-LITGATE gate state: columns waiting on their lighting drain before the
  // first mesh. Keyed with y == 0 (one entry per chunk column).
  world::ColumnLightingGate lightingGate_{};
 std::unordered_set<chunk::ChunkBuilder*> dirtyChunks_{};
 std::unordered_set<chunk::ChunkBuilder*> nearDirtyChunks_{};
 std::vector<std::unordered_set<chunk::ChunkBuilder*>> drawRings_{};
 std::vector<std::vector<chunk::ChunkBuilder*>> visibleDrawRings_{};
 // Scratch for pushCullState/popCullState. Kept resident so the inner vectors
 // retain their capacity across frames.
 std::vector<std::vector<chunk::ChunkBuilder*>> savedVisibleDrawRings_{};
 bool cullStateSaved_ = false;
 std::deque<world::SectionPos> pendingColumns_{};
 std::unordered_set<world::SectionPos, world::SectionPosHash> pendingSet_{};
 // Chunk columns that arrived since the last compileChunks pass. Drained once
 // per frame so a burst of arrivals refreshes each shared border exactly once
 // instead of once per arriving neighbour. Keyed with y == 0.
 std::unordered_set<world::SectionPos, world::SectionPosHash> pendingBorderRefresh_{};
 void drainBorderRefresh();
 std::vector<std::unique_ptr<chunk::ChunkBuilder>> retiring_{};
 chunk::ChunkRegionManager regionManager_{};
 chunk::ChunkMeshScheduler meshScheduler_{};
 std::vector<std::shared_ptr<chunk::ChunkMeshJob>> pendingMeshUploads_{};
 int centerSectionX_ = std::numeric_limits<int>::min();
 int centerSectionZ_ = std::numeric_limits<int>::min();
  int renderRadiusChunks_ = 0;
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
 int occlusionStamp_ = 0;
 std::vector<chunk::ChunkBuilder*> occlusionQueue_{};
 void cameraInterpPosition(double tickDelta, double& x, double& y, double& z) const;
 net::minecraft::Entity* cameraEntity_ = nullptr;
 bool renderCameraEntity_ = false;
 net::minecraft::client::option::GameOptions* options_ = nullptr;
 net::minecraft::client::option::GameOptions defaultOptions_{};
 net::minecraft::client::option::RenderSettings settings_{};
};
} // namespace net::minecraft::client::render
