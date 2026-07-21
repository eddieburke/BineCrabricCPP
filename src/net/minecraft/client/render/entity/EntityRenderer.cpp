#include "net/minecraft/client/render/entity/EntityRenderer.hpp"
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
namespace net::minecraft::client::render::entity {
namespace {
constexpr int kFireBlockId = 51;
}
void EntityRenderer::setDispatcher(EntityRenderDispatcher* dispatcherIn) {
 dispatcher = dispatcherIn;
}
void EntityRenderer::postRender(
    const net::minecraft::Entity& entity, double x, double y, double z, float /*yaw*/, float tickDelta) {
 if(dispatcher == nullptr) {
  return;
 }
 const auto& options = dispatcher->options();
 if(options.fancyGraphics && options.entityShadows && shadowRadius > 0.0f) {
  const double dist = dispatcher->squaredDistanceTo(entity.x, entity.y, entity.z);
  float shade = static_cast<float>((1.0 - dist / 256.0) * static_cast<double>(shadowDarkness));
  if(shade > 0.0f) {
   renderShadow(entity, x, y, z, shade, tickDelta);
  }
 }
 if(entity.isOnFire()) {
  renderOnFire(entity, x, y, z, tickDelta);
 }
}
void EntityRenderer::bindTexture(std::string_view texturePath) {
 if(dispatcher == nullptr || dispatcher->textureManager() == nullptr) {
  return;
 }
 const int textureId = dispatcher->textureManager()->getTextureId(std::string(texturePath));
 RenderSystem::activeTexture(0x84C0);
 RenderSystem::enableTexture();
 RenderSystem::bindTexture(0x0DE1, textureId);
}
bool EntityRenderer::bindDownloadedTexture(std::string_view url) {
  if(dispatcher == nullptr || dispatcher->textureManager() == nullptr) {
   return false;
  }
 if(!url.empty()) {
  if(url.find("Cloak") != std::string::npos || url.find("cape") != std::string::npos) {
   dispatcher->textureManager()->downloadCapeImage(std::string(url));
  } else {
   dispatcher->textureManager()->downloadSkinImage(std::string(url));
  }
  const int textureId = dispatcher->textureManager()->downloadTexture(std::string(url));
  if(textureId < 0) {
   return false;
  }
  RenderSystem::bindTexture(0x0DE1, textureId);
 }
 return true;
}
font::TextRenderer* EntityRenderer::getTextRenderer() const noexcept {
 return dispatcher != nullptr ? dispatcher->getTextRenderer() : nullptr;
}
void EntityRenderer::renderOnFire(
    const net::minecraft::Entity& entity, double dx, double dy, double dz, float tickDelta) {
 (void)tickDelta;
 net::minecraft::block::Block* fireBlock = net::minecraft::block::Block::BLOCKS[kFireBlockId];
 const int texture = fireBlock != nullptr ? fireBlock->textureId : 31;
 int u0 = (texture & 0xF) << 4;
 int v0 = texture & 0xF0;
 float uMin = static_cast<float>(u0) / 256.0f;
 float uMax = (static_cast<float>(u0) + 15.99f) / 256.0f;
 float vMin = static_cast<float>(v0) / 256.0f;
 float vMax = (static_cast<float>(v0) + 15.99f) / 256.0f;
 {
  RenderSystem::pushMatrix();
  RenderSystem::translate(static_cast<float>(dx), static_cast<float>(dy), static_cast<float>(dz));
  const float scale = entity.width * 1.4f;
  RenderSystem::scale(scale, scale, scale);
  bindTexture("/terrain.png");
  Tessellator& tessellator = Tessellator::INSTANCE;
  float radius = 0.35f;
  float offset = 0.0f;
  float remaining = entity.height / scale;
  const float yBase = static_cast<float>(entity.y - entity.boundingBox.minY);
  if(dispatcher != nullptr) {
   RenderSystem::rotate(-dispatcher->yaw_, 0.0f, 1.0f, 0.0f);
  }
  RenderSystem::translate(0.0f, 0.0f, -0.3f + static_cast<float>(static_cast<int>(remaining)) * 0.02f);
  RenderSystem::color4f(1.0f, 1.0f, 1.0f, 1.0f);
  float layerOffset = 0.0f;
  int layer = 0;
  tessellator.startQuads();
  while(remaining > 0.0f) {
   if(layer % 2 == 0) {
    uMin = static_cast<float>(u0) / 256.0f;
    uMax = (static_cast<float>(u0) + 15.99f) / 256.0f;
    vMin = static_cast<float>(v0) / 256.0f;
    vMax = (static_cast<float>(v0) + 15.99f) / 256.0f;
   } else {
    uMin = static_cast<float>(u0) / 256.0f;
    uMax = (static_cast<float>(u0) + 15.99f) / 256.0f;
    vMin = static_cast<float>(v0 + 16) / 256.0f;
    vMax = (static_cast<float>(v0 + 16) + 15.99f) / 256.0f;
   }
   if(layer / 2 % 2 == 0) {
    const float swap = uMax;
    uMax = uMin;
    uMin = swap;
   }
   tessellator.vertex(radius - offset, 0.0f - yBase, layerOffset, uMax, vMax);
   tessellator.vertex(-radius - offset, 0.0f - yBase, layerOffset, uMin, vMax);
   tessellator.vertex(-radius - offset, 1.4f - yBase, layerOffset, uMin, vMin);
   tessellator.vertex(radius - offset, 1.4f - yBase, layerOffset, uMax, vMin);
   remaining -= 0.45f;
   offset -= 0.45f;
   radius *= 0.9f;
   layerOffset += 0.03f;
   ++layer;
  }
  tessellator.draw();
  RenderSystem::popMatrix();
 }
}
World* EntityRenderer::getWorld() const {
 return dispatcher != nullptr ? dispatcher->world() : nullptr;
}
void EntityRenderer::renderShadow(
    const net::minecraft::Entity& entity, double dx, double dy, double dz, float yaw, float tickDelta) {
 render::RenderPassScope passScope(render::RenderType::entityCutout());
 RenderSystem::enableBlend();
 RenderSystem::blendFunc(0x0302, 0x0303);
 if(dispatcher != nullptr && dispatcher->textureManager() != nullptr) {
  const int shadowTex = dispatcher->textureManager()->getTextureId("%clamp%/misc/shadow.png");
  RenderSystem::activeTexture(0x84C0);
  RenderSystem::enableTexture();
  RenderSystem::bindTexture(0x0DE1, shadowTex);
 }
 World* world = getWorld();
 if(world == nullptr) {
  return;
 }
 RenderSystem::depthMask(false);
 const float shadowSize = shadowRadius;
 const double ex = entity.lastTickX + (entity.x - entity.lastTickX) * static_cast<double>(tickDelta);
 const double ey = entity.lastTickY + (entity.y - entity.lastTickY) * static_cast<double>(tickDelta) +
                   static_cast<double>(entity.getShadowRadius());
 const double ez = entity.lastTickZ + (entity.z - entity.lastTickZ) * static_cast<double>(tickDelta);
 const int minX = MathHelper::floor(ex - static_cast<double>(shadowSize));
 const int maxX = MathHelper::floor(ex + static_cast<double>(shadowSize));
 const int minY = MathHelper::floor(ey - static_cast<double>(shadowSize));
 const int maxY = MathHelper::floor(ey);
 const int minZ = MathHelper::floor(ez - static_cast<double>(shadowSize));
 const int maxZ = MathHelper::floor(ez + static_cast<double>(shadowSize));
 const double cx = dx - ex;
 const double cy = dy - ey;
 const double cz = dz - ez;
 Tessellator& tessellator = Tessellator::INSTANCE;
 tessellator.startQuads();
 for(int i = minX; i <= maxX; ++i) {
  for(int j = minY; j <= maxY; ++j) {
   for(int k = minZ; k <= maxZ; ++k) {
    const int blockId = world->getBlockId(i, j - 1, k);
    if(blockId <= 0 || world->getLightLevel(i, j, k) <= 3) {
     continue;
    }
    net::minecraft::block::Block* block = net::minecraft::block::Block::BLOCKS[blockId];
    if(block == nullptr) {
     continue;
    }
    renderShadowOnBlock(*block,
                        dx,
                        dy + static_cast<double>(entity.getShadowRadius()),
                        dz,
                        i,
                        j,
                        k,
                        yaw,
                        shadowSize,
                        cx,
                        cy + static_cast<double>(entity.getShadowRadius()),
                        cz);
   }
  }
 }
 tessellator.draw();
 RenderSystem::color4f(1.0f, 1.0f, 1.0f, 1.0f);
 RenderSystem::disableBlend();
 RenderSystem::depthMask(true);
}
void EntityRenderer::renderShadowOnBlock(net::minecraft::block::Block& block,
                                         double dx,
                                         double dy,
                                         double dz,
                                         int x,
                                         int y,
                                         int z,
                                         float yaw,
                                         float shadowSize,
                                         double cx,
                                         double cy,
                                         double cz) {
 if(!block.receivesEntityShadow()) {
  return;
 }
 Tessellator& tessellator = Tessellator::INSTANCE;
 World* world = getWorld();
 if(world == nullptr) {
  return;
 }
 double shade = (static_cast<double>(yaw) - (dy - (static_cast<double>(y) + cy)) / 2.0) * 0.5 *
                static_cast<double>(world->getLightBrightness(x, y, z));
 if(shade < 0.0) {
  return;
 }
 if(shade > 1.0) {
  shade = 1.0;
 }
 tessellator.color(1.0f, 1.0f, 1.0f, static_cast<float>(shade));
 const double x1 = static_cast<double>(x) + block.minX + cx;
 const double x2 = static_cast<double>(x) + block.maxX + cx;
 // The block was sampled one below y; rest the quad on its actual top face
 // (identical to y + minY for full cubes, correct for shorter mod blocks).
 const double yPlane = static_cast<double>(y - 1) + block.maxY + cy + 0.015625;
 const double z1 = static_cast<double>(z) + block.minZ + cz;
 const double z2 = static_cast<double>(z) + block.maxZ + cz;
 const float u1 = static_cast<float>((dx - x1) / 2.0 / static_cast<double>(shadowSize) + 0.5);
 const float u2 = static_cast<float>((dx - x2) / 2.0 / static_cast<double>(shadowSize) + 0.5);
 const float v1 = static_cast<float>((dz - z1) / 2.0 / static_cast<double>(shadowSize) + 0.5);
 const float v2 = static_cast<float>((dz - z2) / 2.0 / static_cast<double>(shadowSize) + 0.5);
 tessellator.vertex(x1, yPlane, z1, u1, v1);
 tessellator.vertex(x1, yPlane, z2, u1, v2);
 tessellator.vertex(x2, yPlane, z2, u2, v2);
 tessellator.vertex(x2, yPlane, z1, u2, v1);
}
void EntityRenderer::renderShape(const Box& box, double x, double y, double z) {
 RenderSystem::disableTexture();
 Tessellator& tessellator = Tessellator::INSTANCE;
 RenderSystem::color4f(1.0f, 1.0f, 1.0f, 1.0f);
 tessellator.startQuads();
 tessellator.translate(x, y, z);
 tessellator.normal(0.0f, 0.0f, -1.0f);
 tessellator.vertex(box.minX, box.maxY, box.minZ);
 tessellator.vertex(box.maxX, box.maxY, box.minZ);
 tessellator.vertex(box.maxX, box.minY, box.minZ);
 tessellator.vertex(box.minX, box.minY, box.minZ);
 tessellator.normal(0.0f, 0.0f, 1.0f);
 tessellator.vertex(box.minX, box.minY, box.maxZ);
 tessellator.vertex(box.maxX, box.minY, box.maxZ);
 tessellator.vertex(box.maxX, box.maxY, box.maxZ);
 tessellator.vertex(box.minX, box.maxY, box.maxZ);
 tessellator.normal(0.0f, -1.0f, 0.0f);
 tessellator.vertex(box.minX, box.minY, box.minZ);
 tessellator.vertex(box.maxX, box.minY, box.minZ);
 tessellator.vertex(box.maxX, box.minY, box.maxZ);
 tessellator.vertex(box.minX, box.minY, box.maxZ);
 tessellator.normal(0.0f, 1.0f, 0.0f);
 tessellator.vertex(box.minX, box.maxY, box.maxZ);
 tessellator.vertex(box.maxX, box.maxY, box.maxZ);
 tessellator.vertex(box.maxX, box.maxY, box.minZ);
 tessellator.vertex(box.minX, box.maxY, box.minZ);
 tessellator.normal(-1.0f, 0.0f, 0.0f);
 tessellator.vertex(box.minX, box.minY, box.maxZ);
 tessellator.vertex(box.minX, box.maxY, box.maxZ);
 tessellator.vertex(box.minX, box.maxY, box.minZ);
 tessellator.vertex(box.minX, box.minY, box.minZ);
 tessellator.normal(1.0f, 0.0f, 0.0f);
 tessellator.vertex(box.maxX, box.minY, box.minZ);
 tessellator.vertex(box.maxX, box.maxY, box.minZ);
 tessellator.vertex(box.maxX, box.maxY, box.maxZ);
 tessellator.vertex(box.maxX, box.minY, box.maxZ);
 tessellator.translate(0.0, 0.0, 0.0);
 tessellator.draw();
 RenderSystem::enableTexture();
}
void EntityRenderer::renderShapeFlat(const Box& box) {
 Tessellator& tessellator = Tessellator::INSTANCE;
 tessellator.startQuads();
 tessellator.vertex(box.minX, box.maxY, box.minZ);
 tessellator.vertex(box.maxX, box.maxY, box.minZ);
 tessellator.vertex(box.maxX, box.minY, box.minZ);
 tessellator.vertex(box.minX, box.minY, box.minZ);
 tessellator.vertex(box.minX, box.minY, box.maxZ);
 tessellator.vertex(box.maxX, box.minY, box.maxZ);
 tessellator.vertex(box.maxX, box.maxY, box.maxZ);
 tessellator.vertex(box.minX, box.maxY, box.maxZ);
 tessellator.vertex(box.minX, box.minY, box.minZ);
 tessellator.vertex(box.maxX, box.minY, box.minZ);
 tessellator.vertex(box.maxX, box.minY, box.maxZ);
 tessellator.vertex(box.minX, box.minY, box.maxZ);
 tessellator.vertex(box.minX, box.maxY, box.maxZ);
 tessellator.vertex(box.maxX, box.maxY, box.maxZ);
 tessellator.vertex(box.maxX, box.maxY, box.minZ);
 tessellator.vertex(box.minX, box.maxY, box.minZ);
 tessellator.vertex(box.minX, box.minY, box.maxZ);
 tessellator.vertex(box.minX, box.maxY, box.maxZ);
 tessellator.vertex(box.minX, box.maxY, box.minZ);
 tessellator.vertex(box.minX, box.minY, box.minZ);
 tessellator.vertex(box.maxX, box.minY, box.minZ);
 tessellator.vertex(box.maxX, box.maxY, box.minZ);
 tessellator.vertex(box.maxX, box.maxY, box.maxZ);
 tessellator.vertex(box.maxX, box.minY, box.maxZ);
 tessellator.draw();
}
} // namespace net::minecraft::client::render::entity
