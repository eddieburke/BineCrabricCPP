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
class FrustumCuller;
class WorldRenderer : public net::minecraft::GameEventListener {
 friend class GameRenderer;
 friend class ChunkSectionSystem;
 friend class ChunkCompilePipeline;

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
  chunkSections_.resetSectionFrontier();
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
 void renderChunks(int layer, double tickDelta, bool drawModMeshes = true, bool skipBuildDrawLists = false);
 int renderChunksVbo(
     int layer, double tickDelta, double interpX, double interpY, double interpZ, bool skipBuildDrawLists = false);
 int renderModChunkMeshes(int layer, double interpX, double interpY, double interpZ);
 void cameraInterpPosition(double tickDelta, double& x, double& y, double& z) const;
 int lastDrawnRegionCount_ = 0;
 int entityRenderCooldown = 2;
 int entityCount = 0;
 int renderedEntityCount = 0;
 int culledEntityCount = 0;
 net::minecraft::Entity* cameraEntity_ = nullptr;
 bool renderCameraEntity_ = false;
 net::minecraft::client::option::GameOptions* options_ = nullptr;
 net::minecraft::client::option::GameOptions defaultOptions_{};
 net::minecraft::client::option::RenderSettings settings_{};
 ChunkSectionSystem chunkSections_{*this};
 ChunkCompilePipeline compilePipeline_{*this};
};
} // namespace net::minecraft::client::render
