#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/RailBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/mod/ModClient.hpp"
#include "net/minecraft/mod/ModTexture.hpp"
#include "net/minecraft/client/render/block/BlockRenderType.hpp"
namespace net::minecraft::client::render::block {
namespace option = net::minecraft::client::option;
namespace {
net::minecraft::block::Block* blockAt(int blockId) {
  if(blockId < 0 || blockId >= net::minecraft::block::Block::BLOCK_COUNT) {
    return nullptr;
  }
  return net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
}
} // namespace
void BlockRenderManager::snapshotGlobals() {
  ctx.fancyGraphics = fancyGraphics;
  if(Minecraft::INSTANCE != nullptr) {
    ctx.opts = option::resolve(Minecraft::INSTANCE->options);
  }
}
option::ResolvedRenderOptions BlockRenderManager::blockRenderManagerOptionsSnapshot() {
  if(Minecraft::INSTANCE != nullptr) {
    return option::resolve(Minecraft::INSTANCE->options);
  }
  return {};
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
void BlockRenderManager::renderWithTexture(net::minecraft::block::Block& block, int x, int y, int z,
                                           int textureOverrideIn) {
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
  if(net::minecraft::mod::drawBlockWorld(*this, block, x, y, z)) {
    return true;
  }
  const int renderType = block.getRenderType();
  // Bounds live on the context, not the Block singleton: mesh workers and
  // the main-thread tick must never race on Block::minX..maxZ.
  ctx.renderBounds = block.getRenderBounds(ctx.blockView, x, y, z);
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
    return false;
  }
}
void BlockRenderManager::renderExtendedPiston(net::minecraft::block::Block& block, int x, int y, int z) {
  piston_.renderExtendedPiston(block, x, y, z);
}
void BlockRenderManager::renderPistonHeadWithoutCulling(net::minecraft::block::Block& block, int x, int y, int z,
                                                        bool extendedHalfway) {
  piston_.renderPistonHeadWithoutCulling(block, x, y, z, extendedHalfway);
}
void BlockRenderManager::render(net::minecraft::block::Block& block, int metadata, float brightness) {
  if(net::minecraft::mod::isMod(block.textureId)) {
    client::texture::TextureManager* tm = ctx.textureManager;
    if(tm == nullptr && Minecraft::INSTANCE != nullptr) {
      tm = &Minecraft::INSTANCE->textureManager;
    }
    if(tm != nullptr) {
      net::minecraft::mod::bind(*tm, block.textureId);
    }
  }
  if(net::minecraft::mod::drawBlockInventory(*this, block, metadata, brightness)) {
    return;
  }
  inventory_.render(block, metadata, brightness);
}
bool BlockRenderManager::renderStandardBlock(net::minecraft::block::Block& block, int x, int y, int z) {
  return cube_.renderBlock(block, x, y, z);
}
void BlockRenderManager::renderFallingBlockEntity(net::minecraft::block::Block& block, net::minecraft::World* world,
                                                  int x, int y, int z) {
  falling_.renderFallingBlockEntity(block, world, x, y, z);
}
bool BlockRenderManager::isSideLit(int renderType) {
  return renderType == BlockRenderType::FULL_CUBE || renderType == BlockRenderType::CACTUS ||
         renderType == BlockRenderType::STAIRS || renderType == BlockRenderType::FENCE ||
         renderType == BlockRenderType::PISTON;
}
} // namespace net::minecraft::client::render::block
