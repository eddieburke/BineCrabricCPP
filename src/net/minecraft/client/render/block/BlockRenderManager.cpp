#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/RailBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/TextureResolve.hpp"
#include "net/minecraft/client/render/block/BlockRenderType.hpp"
#include "net/minecraft/client/render/chunk/RegionSnapshot.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/mod/ModClient.hpp"
#include "net/minecraft/registry/TextureRegistry.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/WorldRegion.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/light/LightType.hpp"
namespace net::minecraft::client::render::block {
namespace option = net::minecraft::client::option;
void BlockRenderContext::resolveLightSource() {
 lightRegion = dynamic_cast<const net::minecraft::WorldRegion*>(blockView);
 lightWorld = lightRegion != nullptr ? nullptr : dynamic_cast<const net::minecraft::World*>(blockView);
 lightSnapshot = lightRegion == nullptr && lightWorld == nullptr
                     ? dynamic_cast<const chunk::RegionSnapshot*>(blockView)
                     : nullptr;
}
void BlockRenderContext::sampleFaceLight(int x, int y, int z) {
 // Chunk::index packs y into seven bits, so y = 128 (the face above a block at
 // the build limit) aliases into the z field and reads a different column.
 y = std::clamp(y, 0, net::minecraft::Chunk::height - 1);
 if(lightRegion != nullptr) {
  faceBlockLight = lightRegion->getBlockLight(x, y, z);
  faceSkyLight = lightRegion->getSkyLight(x, y, z);
 } else if(lightSnapshot != nullptr) {
  faceBlockLight = lightSnapshot->getBlockLight(x, y, z);
  faceSkyLight = lightSnapshot->getSkyLight(x, y, z);
 } else if(lightWorld != nullptr) {
  faceBlockLight = lightWorld->getBrightness(net::minecraft::LightType::Block, x, y, z);
  faceSkyLight = lightWorld->getBrightness(net::minecraft::LightType::Sky, x, y, z);
 } else {
  faceBlockLight = 15;
  faceSkyLight = 15;
 }
 faceBlockLight = std::max(faceBlockLight, blockEmission);
}
void BlockRenderContext::sampleSurroundingLight(int x, int y, int z) {
 static constexpr int kOffsets[6][3] = {{0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}};
 sampleFaceLight(x, y, z);
 int bestBlock = faceBlockLight;
 int bestSky = faceSkyLight;
 for(const auto& offset : kOffsets) {
  sampleFaceLight(x + offset[0], y + offset[1], z + offset[2]);
  bestBlock = std::max(bestBlock, faceBlockLight);
  bestSky = std::max(bestSky, faceSkyLight);
 }
 faceBlockLight = bestBlock;
 faceSkyLight = bestSky;
}
namespace {
std::atomic_bool g_voxelizeLightBlocks = false;
net::minecraft::block::Block* blockAt(int blockId) {
 if(blockId < 0 || blockId >= net::minecraft::block::Block::BLOCK_COUNT) {
  return nullptr;
 }
 return net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
}
} // namespace
void BlockRenderManager::setVoxelizeLightBlocks(bool enabled) noexcept {
 g_voxelizeLightBlocks.store(enabled, std::memory_order_relaxed);
}
void BlockRenderManager::snapshotGlobals(const option::RenderSettings* overrideSettings) {
 if(overrideSettings != nullptr) {
  ctx.opts = *overrideSettings;
  return;
 }
 if(Minecraft::INSTANCE == nullptr) {
  return;
 }
 if(Minecraft::INSTANCE->gameRenderer != nullptr) {
  ctx.opts = Minecraft::INSTANCE->gameRenderer->frameSettings();
 } else {
  ctx.opts = option::renderSettings(Minecraft::INSTANCE->options);
 }
}
void BlockRenderManager::renderWithTexture(int blockId, int x, int y, int z, int textureOverrideIn) {
 if(net::minecraft::block::Block* b = blockAt(blockId)) {
  renderWithTexture(*b, x, y, z, textureOverrideIn);
 }
}
void BlockRenderManager::renderWithoutCulling(int blockId, int x, int y, int z) {
 if(net::minecraft::block::Block* b = blockAt(blockId)) {
  renderWithoutCulling(*b, x, y, z);
 }
}
bool BlockRenderManager::render(int blockId, int x, int y, int z) {
 if(ctx.blockView == nullptr) {
  return false;
 }
 net::minecraft::block::Block* b = blockAt(blockId);
 if(b == nullptr) {
  return false;
 }
 return render(*b, x, y, z);
}
void BlockRenderManager::render(int blockId, int metadata, float brightness) {
 if(net::minecraft::block::Block* b = blockAt(blockId)) {
  render(*b, metadata, brightness);
 }
}
void BlockRenderManager::renderExtendedPiston(int blockId, int x, int y, int z) {
 if(net::minecraft::block::Block* b = blockAt(blockId)) {
  renderExtendedPiston(*b, x, y, z);
 }
}
void BlockRenderManager::renderPistonHeadWithoutCulling(int blockId, int x, int y, int z, bool extendedHalfway) {
 if(net::minecraft::block::Block* b = blockAt(blockId)) {
  renderPistonHeadWithoutCulling(*b, x, y, z, extendedHalfway);
 }
}
bool BlockRenderManager::renderBlock(int blockId, int x, int y, int z) {
 if(net::minecraft::block::Block* b = blockAt(blockId)) {
  ctx.renderBounds = b->getRenderBounds(ctx.blockView, x, y, z);
  return cube_.renderBlock(*b, x, y, z);
 }
 return false;
}
void BlockRenderManager::renderWithTexture(
    net::minecraft::block::Block& block, int x, int y, int z, int textureOverrideIn) {
 ctx.textureOverride = textureOverrideIn;
 render(block, x, y, z);
 ctx.textureOverride = -1;
}
void BlockRenderManager::renderWithoutCulling(net::minecraft::block::Block& block, int x, int y, int z) {
 ctx.skipFaceCulling = true;
 render(block, x, y, z);
 ctx.skipFaceCulling = false;
}
bool BlockRenderManager::render(net::minecraft::block::Block& block, int x, int y, int z) {
 ctx.faceState.useAo = false;
 ctx.blockX = x;
 ctx.blockY = y;
 ctx.blockZ = z;
 ctx.blockEmission = block.emission();
 ctx.resolveLightSource();
 // Renderers that know their face override this per face; the ones that don't —
 // baked models, mod blocks, the item path — keep this value, so it has to be
 // the light reaching the block's surfaces rather than the nothing stored inside
 // a solid block.
 ctx.sampleSurroundingLight(x, y, z);
 ctx.blockLight = ctx.faceBlockLight;
 ctx.skyLight = ctx.faceSkyLight;
 ctx.blockId = resolveShaderBlockId(block.id);
 ctx.blockFluid = block.getRenderType() == BlockRenderType::FLUID;
 ctx.blockMetadata = ctx.blockView->getBlockMeta(x, y, z);
 if(ctx.tess != nullptr)
  ctx.tess->blockData(x, y, z, ctx.blockEmission, ctx.blockLight, ctx.skyLight, ctx.blockId, ctx.blockFluid,
                      ctx.blockMetadata);
 // Bounds live on the context, not the Block singleton: mesh workers and
 // the main-thread tick must never race on Block::minX..maxZ. They are set
 // before the mod hook because baked models cull their faces through
 // isSideVisibleForBounds, which reads them.
 ctx.renderBounds = block.getRenderBounds(ctx.blockView, x, y, z);
 if(ctx.textureOverride < 0 && net::minecraft::mod::drawBlockWorld(*this, block, x, y, z)) {
  return true;
 }
 const int renderType = block.getRenderType();
 switch(renderType) {
 case BlockRenderType::FULL_CUBE:
  return cube_.renderBlock(block, x, y, z);
 case BlockRenderType::FLUID:
  if(&block.material == &::net::minecraft::block::material::Material::WATER && !ctx.opts.renderWater) {
   return false;
  }
  return fluid_.renderFluid(block, x, y, z);
 case BlockRenderType::CACTUS:
  return cube_.renderCactus(block, x, y, z);
 case BlockRenderType::CROSS:
  return cross_.render(block, x, y, z);
 case BlockRenderType::CROP:
  return crop_.render(block, x, y, z);
 case BlockRenderType::TORCH:
  return torch_.render(block, x, y, z);
 case BlockRenderType::FIRE:
  return fire_.render(block, x, y, z);
 case BlockRenderType::REDSTONE_DUST:
  return redstoneDust_.render(block, x, y, z);
 case BlockRenderType::LADDER:
  return ladder_.render(block, x, y, z);
 case BlockRenderType::DOOR:
  return door_.render(block, x, y, z);
 case BlockRenderType::RAIL:
  return rail_.render(static_cast<net::minecraft::block::RailBlock&>(block), x, y, z);
 case BlockRenderType::STAIRS:
  return stairs_.render(block, x, y, z);
 case BlockRenderType::FENCE:
  return fence_.render(block, x, y, z);
 case BlockRenderType::LEVER:
  return lever_.render(block, x, y, z);
 case BlockRenderType::BED:
  return bed_.render(block, x, y, z);
 case BlockRenderType::REPEATER:
  return repeater_.render(block, x, y, z);
 case BlockRenderType::PISTON:
  return piston_.renderPiston(block, x, y, z, false);
 case BlockRenderType::PISTON_HEAD:
  return piston_.renderPistonHead(block, x, y, z, true);
 default:
  if(g_voxelizeLightBlocks.load(std::memory_order_relaxed) && renderType < 0 && block.emission() > 0 &&
     !block.isOpaque()) {
   Tessellator& tessellator = ctx.activeTess(block.textureId);
   tessellator.blockData(x + 0.5,
                         y + 0.5,
                         z + 0.5,
                         block.emission(),
                         block.emission(),
                         block.emission(),
                         ctx.blockId,
                         ctx.blockFluid,
                         ctx.blockMetadata);
   tessellator.texture(0.0, 0.0);
   for(int i = 0; i < 4; ++i) tessellator.vertex(x + 0.5, y + 0.5, z + 0.5);
   return true;
  }
  return false;
 }
}
void BlockRenderManager::renderExtendedPiston(net::minecraft::block::Block& block, int x, int y, int z) {
 piston_.renderExtendedPiston(block, x, y, z);
}
void BlockRenderManager::renderPistonHeadWithoutCulling(
    net::minecraft::block::Block& block, int x, int y, int z, bool extendedHalfway) {
 piston_.renderPistonHeadWithoutCulling(block, x, y, z, extendedHalfway);
}
void BlockRenderManager::render(net::minecraft::block::Block& block, int metadata, float brightness) {
 // Inventory/dropped-item draws always run on the main thread with a live
 // texture manager. Adopting it matters beyond the bind below: a baked model
 // with several textures rebinds per batch via ctx.bindTextureFor, which is a
 // no-op while ctx.textureManager is null, so every batch would otherwise be
 // drawn with whatever the caller happened to bind.
 if(ctx.textureManager == nullptr && Minecraft::INSTANCE != nullptr) {
  ctx.textureManager = &Minecraft::INSTANCE->textureManager;
 }
 if(ctx.textureManager != nullptr) {
  ctx.bindTextureFor(block.textureId);
 }
 if(net::minecraft::mod::drawBlockInventory(*this, block, metadata, brightness)) {
  return;
 }
 inventory_.render(block, metadata, brightness);
}
bool BlockRenderManager::renderStandardBlock(net::minecraft::block::Block& block, int x, int y, int z) {
 return cube_.renderBlock(block, x, y, z);
}
void BlockRenderManager::renderFallingBlockEntity(
    net::minecraft::block::Block& block, net::minecraft::World* world, int x, int y, int z) {
 falling_.renderFallingBlockEntity(block, world, x, y, z);
}
bool BlockRenderManager::isSideLit(int renderType) {
 return renderType == BlockRenderType::FULL_CUBE || renderType == BlockRenderType::CACTUS ||
        renderType == BlockRenderType::STAIRS || renderType == BlockRenderType::FENCE ||
        renderType == BlockRenderType::PISTON;
}
} // namespace net::minecraft::client::render::block
