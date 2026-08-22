#pragma once
#include <limits>
#include <memory>
#include <string>
#include <vector>
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/chunk/ChunkBuilder.hpp"
#include "net/minecraft/client/render/world/ChunkCompilePipeline.hpp"
#include "net/minecraft/client/render/world/ChunkSectionSystem.hpp"
#include "net/minecraft/client/render/world/TerrainScene.hpp"
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
class Frustum;
class WorldRenderer : public net::minecraft::GameEventListener {
 public:
 WorldRenderer(net::minecraft::client::Minecraft* minecraft = nullptr,
               net::minecraft::client::texture::TextureManager* textureManager = nullptr);
 void setWorld(net::minecraft::World* world);
 void reload();
 // Entities compose their poses onto the iris per-draw base (core::drawModelView)
 // instead of a separately-maintained camera stack; the projection is the iris draw
  // projection. cameraPos is the frame camera position (core::cameraFrame).
 void renderEntities(const Vec3d& cameraPos, Frustum* culler, float tickDelta);
 [[nodiscard]] std::string getEntityDebugInfo() const;
 int render(net::minecraft::LivingEntity& camera, int layer, bool drawModMeshes = true);
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
 float miningProgress = 0.0f;
 net::minecraft::client::Minecraft* client = nullptr;
 net::minecraft::World* world = nullptr;
 net::minecraft::client::texture::TextureManager* textureManager = nullptr;
  net::minecraft::client::render::block::BlockRenderManager blockRenderManager{Tessellator::INSTANCE};
 void setCamera(net::minecraft::Entity* camera) {
  cameraEntity_ = camera;
  // The section system and compile pipeline read the scene, not this class;
  // refresh it together with the camera so every pass downstream of
  // GameRenderer::setCamera sees the current camera, settings and options
  // (frameSettings_ is captured before rendering in onFrameUpdate).
  scene_.camera = camera;
  scene_.settings = &frameSettings();
  scene_.options = &activeOptions();
 }
 [[nodiscard]] net::minecraft::Entity* cameraEntity() const noexcept {
  return cameraEntity_;
 }
 void setRenderCameraEntity(bool renderCameraEntity) noexcept {
  renderCameraEntity_ = renderCameraEntity;
 }
 [[nodiscard]] bool renderCameraEntity() const noexcept {
  return renderCameraEntity_;
 }
 void setOptions(net::minecraft::client::option::GameOptions* options) {
  options_ = options;
 }
 // One accessor instead of a forwarding method per operation. WorldRenderer used
 // to carry a pass-through for cullChunks / compileChunks / releaseSections /
 // getChunkDebugInfo / reloadIfViewDistanceChanged / the scoped-cull flag, each
 // adding a name to grep through and a place for the two sides to disagree.
 [[nodiscard]] ChunkSectionSystem& sections() noexcept {
  return chunkSections_;
 }
 [[nodiscard]] const ChunkSectionSystem& sections() const noexcept {
  return chunkSections_;
 }
 [[nodiscard]] ChunkCompilePipeline& compiler() noexcept {
  return compilePipeline_;
 }
 // RAII, like core::ScopedDrawCameraState. The nested shadow/portal pass used to
 // hand-balance the flag around an early-returning body.
 class ScopedCullState {
public:
  explicit ScopedCullState(WorldRenderer& renderer) : renderer_(&renderer) {
   renderer_->chunkSections_.useScopedVisibleSections(true);
  }
  ~ScopedCullState() {
   renderer_->chunkSections_.useScopedVisibleSections(false);
  }
  ScopedCullState(const ScopedCullState&) = delete;
  ScopedCullState& operator=(const ScopedCullState&) = delete;

private:
  WorldRenderer* renderer_;
 };
 // The two render origins, one per published model view. Both are real; which one
 // a draw uses is decided by which matrix it composes onto, so name the choice at
 // every call site rather than reaching for "the camera position".
 //
 // sectionOrigin  = Camera.getPosition() (eye). Geometry uploaded as `worldPos - eye`
 //                  composes onto sectionLocalModelView. Chunk sections, chunk
 //                  culling and the mod mesh draw lists.
 // matrixStackOrigin = the camera ENTITY position. Geometry uploaded relative to it
 //                  composes onto fullModelView, which is sectionLocalModelView with
 //                  translate(pos - eye) folded in. Entity/block-entity dispatch,
 //                  the block-damage overlay and the block outline.
 // see src/net/minecraft/client/render/RenderCore.cpp setDrawCameraStateFromCamera
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CameraUniforms.java
 [[nodiscard]] static Vec3d sectionOrigin() noexcept;

 private:
 void renderOutline(const net::minecraft::Box& box);
 [[nodiscard]] net::minecraft::client::option::GameOptions& activeOptions() const;
 // Authoritative per-frame settings captured by GameRenderer::frameSettings_
 // (the C++ equivalent of Iris' CapturedRenderingState). All renderers query
 // this; none re-evaluate settings from raw GameOptions.
 // see third_party/mcp/iris/common/src/main/java/net/irisshaders/iris/pipeline/CapturedRenderingState.java
 [[nodiscard]] const option::RenderSettings& frameSettings() const;
 struct ModMeshDraw {
  const chunk::ModChunkMesh* mesh = nullptr;
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
 };
 struct TerrainRegionDraw {
  chunk::TerrainRegion* region = nullptr;
  std::vector<const chunk::TerrainAllocation*> allocations{};
 };
 // replay submits the batches a previous call already built for this layer,
 // for the fancyGraphics translucent prepass/blend pair.
 int renderChunkLayer(int layer, bool replay, bool drawModMeshes);
 void matrixStackOrigin(double& x, double& y, double& z) const;
 int entityRenderCooldown = 2;
 int entityCount = 0;
 int renderedEntityCount = 0;
 int culledEntityCount = 0;
 net::minecraft::Entity* cameraEntity_ = nullptr;
 bool renderCameraEntity_ = false;
 net::minecraft::client::option::GameOptions* options_ = nullptr;
 // The scene both terrain systems read. Declared before them: ChunkBuilder
 // keeps a pointer to scene_.blockEntities, so the scene must outlive every
 // section (member destruction runs in reverse declaration order).
 TerrainScene scene_{};
 ChunkSectionSystem chunkSections_{scene_};
 // Construction is mutually circular (each system takes the scene alone and
 // reaches its peer through the setter); the peers are wired in the constructor.
 ChunkCompilePipeline compilePipeline_{scene_};
 std::vector<ModMeshDraw> modMeshDraws_{};
 std::vector<TerrainRegionDraw> terrainRegionDraws_{};
 std::size_t terrainRegionDrawCount_ = 0;
 // Layer the cached batches belong to, or -1 when they hold nothing safe to
 // replay (no build yet, or the regions behind them were freed).
 int terrainRegionDrawLayer_ = -1;
};
} // namespace net::minecraft::client::render
