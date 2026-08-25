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
void BlockRenderContext::sampleFaceLightGeneric(int x, int y, int z) {
 sampleFaceLightWith(x, y, z, blockIdAt(x, y, z),
                     [this](int sampleX, int sampleY, int sampleZ, int& blockLight, int& skyLight) {
                      if(lightRegion != nullptr) {
                       blockLight = lightRegion->getBlockLight(sampleX, sampleY, sampleZ);
                       skyLight = lightRegion->getSkyLight(sampleX, sampleY, sampleZ);
                      } else if(lightWorld != nullptr) {
                       blockLight =
                           lightWorld->getBrightness(net::minecraft::LightType::Block, sampleX, sampleY, sampleZ);
                       skyLight =
                           lightWorld->getBrightness(net::minecraft::LightType::Sky, sampleX, sampleY, sampleZ);
                      } else {
                       blockLight = 15;
                       skyLight = 15;
                      }
                     });
}
void BlockRenderContext::sampleSurroundingLight(int x, int y, int z) {
 sampleFaceLight(x, y, z);
 int bestBlock = faceBlockLight;
 int bestSky = faceSkyLight;
 for(const auto& offset : kFaceOffsets) {
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
void BlockRenderManager::renderWithoutCulling(int blockId, int x, int y, int z) {
 if(net::minecraft::block::Block* b = blockAt(blockId)) {
  renderWithoutCulling(*b, x, y, z);
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
 ctx.blockId = resolveShaderBlockId(block.id);
 const int renderType = block.getRenderType();
 ctx.blockFluid = renderType == BlockRenderType::FLUID;
 ctx.blockMetadata = ctx.blockView->getBlockMeta(x, y, z);
 ctx.surroundingLightPending = ctx.tess != nullptr;
 ctx.renderBounds = block.getRenderBounds(ctx.blockView, x, y, z);
 if(ctx.textureOverride < 0 && net::minecraft::mod::drawBlockWorld(*this, block, x, y, z)) {
  return true;
 }
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
   tessellator.blockData(x,
                         y,
                         z,
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
 // An inventory-style block model has no world position, so it cannot sample a
 // face light of its own: it takes the one the caller set with Tessellator::light().
 // activeTess() pushes ctx.faceBlockLight/faceSkyLight into blockData() for every
 // vertex, so leaving them at the (15, 15) default drew every held item, dropped
 // stack, TNT entity and chest minecart at full BLOCK light -- pure torch colour
 // under any pack, which at night is a yellow item in a blue world. The callers
 // already publish the right light (EntityRenderDispatcher::render,
 // HeldItemRenderer::render); GUI icons ask for (15, 15) explicitly.
 ctx.faceBlockLight = static_cast<int>(std::lround(ctx.tess->blockLight()));
 ctx.faceSkyLight = static_cast<int>(std::lround(ctx.tess->skyLight()));
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
