#include "net/minecraft/client/render/atmosphere/CloudRenderer.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/culling/Frustum.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereContext.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
namespace net::minecraft::client::render::atmosphere {
namespace {
[[nodiscard]] Vec3d cloudColorForWorld(net::minecraft::World* world, float tickDelta) {
 if(world == nullptr) {
  return {1.0, 1.0, 1.0};
 }
 return world->getCloudColor(tickDelta);
}
class CloudMask {
 public:
 [[nodiscard]] bool opaque(int x, int z) const noexcept {
  if(size_ <= 0) {
   return true;
  }
  const int mx = ((x % size_) + size_) % size_;
  const int mz = ((z % size_) + size_) % size_;
  return mask_[static_cast<std::size_t>(mz) * static_cast<std::size_t>(size_) +
               static_cast<std::size_t>(mx)] != 0;
 }
 [[nodiscard]] static const CloudMask& forTexture(net::minecraft::client::texture::TextureManager& textures, int textureId) {
  static CloudMask cached;
  static int cachedId = -1;
  if(cachedId == textureId) {
   return cached;
  }
  cachedId = textureId;
  cached = CloudMask();
  net::minecraft::client::texture::RasterImage decoded =
      textures.loadRasterForResource("/environment/clouds.png");
  const net::minecraft::client::texture::RasterImage* image =
      decoded.width > 0 ? &decoded : textures.getRasterImage(textureId);
  if(image == nullptr || image->width <= 0 || image->width != image->height ||
     image->argb.size() < static_cast<std::size_t>(image->width) * static_cast<std::size_t>(image->height)) {
   return cached;
  }
  cached.size_ = image->width;
  cached.mask_.resize(image->argb.size());
  std::size_t opaqueCells = 0;
  for(std::size_t i = 0; i < image->argb.size(); ++i) {
   cached.mask_[i] = (image->argb[i] >> 24) >= 128u ? 1 : 0;
   opaqueCells += cached.mask_[i];
  }
  return cached;
 }

 private:
 int size_ = 0;
 std::vector<std::uint8_t> mask_;
};
} // namespace
namespace {
void renderFancyClouds(const AtmosphereContext& ctx, float tickDelta) {
 if(ctx.camera == nullptr) {
  return;
 }
 core::disableCull();
 const float cameraY = static_cast<float>(ctx.camera->lastTickY +
                                          (ctx.camera->y - ctx.camera->lastTickY) * static_cast<double>(tickDelta));
 Tessellator& tessellator = Tessellator::INSTANCE;
 constexpr float cloudScale = 12.0f;
 constexpr float cloudThickness = 4.0f;
 double cloudX = (ctx.camera->prevX + (ctx.camera->x - ctx.camera->prevX) * static_cast<double>(tickDelta) +
                  static_cast<double>((static_cast<float>(ctx.atmosphereTicks) + tickDelta) * 0.03f)) /
                 static_cast<double>(cloudScale);
 double cloudZ = (ctx.camera->prevZ + (ctx.camera->z - ctx.camera->prevZ) * static_cast<double>(tickDelta)) /
                     static_cast<double>(cloudScale) +
                 0.33;
 const float cloudHeight = client::option::cloudHeightOffset(
     ctx.world->dimension->getCloudHeight() - cameraY + 0.33f, ctx.settings);
 const int originX = MathHelper::floor(cloudX / 2048.0);
 const int originZ = MathHelper::floor(cloudZ / 2048.0);
 cloudX -= static_cast<double>(originX * 2048);
 cloudZ -= static_cast<double>(originZ * 2048);
 const int cloudTexture = ctx.textureManager->getTextureId("/environment/clouds.png");
 ctx.textureManager->bindTexture(cloudTexture);
 const CloudMask& mask = CloudMask::forTexture(*ctx.textureManager, cloudTexture);
 Vec3d cloudColor = cloudColorForWorld(ctx.world, tickDelta);
 float red = static_cast<float>(cloudColor.x);
 float green = static_cast<float>(cloudColor.y);
 float blue = static_cast<float>(cloudColor.z);
 const int texelOriginX = MathHelper::floor(cloudX);
 const int texelOriginZ = MathHelper::floor(cloudZ);
 float texOffsetX = static_cast<float>(texelOriginX) * 0.00390625f;
 float texOffsetZ = static_cast<float>(texelOriginZ) * 0.00390625f;
 const float fracX = static_cast<float>(cloudX - static_cast<double>(texelOriginX));
 const float fracZ = static_cast<float>(cloudZ - static_cast<double>(texelOriginZ));
 constexpr float texScale = 0.00390625f;
 constexpr float edgeInset = 9.765625E-4f;
 constexpr int tileSize = 8;
 constexpr int tileRadius = 3;
 constexpr int cellMin = (-tileRadius + 1) * tileSize;
 constexpr int cellMax = tileRadius * tileSize;
 const core::ScopedDrawCameraState cloudGuard;
 net::minecraft::util::math::Matrix4f cloudPose = core::drawPose();
 cloudPose.scale(cloudScale, 1.0f, cloudScale);
 core::setDrawPose(cloudPose);
 const bool drawBottom = cloudHeight > -cloudThickness - 1.0f;
 const bool drawTop = cloudHeight <= cloudThickness + 1.0f;
 // 40x40 cells, two passes, up to six quads each -- the heaviest immediate-mode
 // geometry vanilla builds. cloudPose (a 12x horizontal scale) is baked into each
 // vertex by the Tessellator, so it has to be folded in here for the cell
 // coordinates below to be in the frustum's space.
 Frustum cellFrustum;
 const bool cullCells = core::drawCameraStateValid();
 if(cullCells) {
  net::minecraft::util::math::Matrix4f posedModelView = core::drawModelView();
  posedModelView.multiply(cloudPose);
  cellFrustum.compute(core::drawProjection(), posedModelView, 0.0, 0.0, 0.0);
 }
 for(int pass = 0; pass < 2; ++pass) {
  const bool colorWrite = pass != 0;
  core::colorMask(colorWrite, colorWrite, colorWrite, colorWrite);
  tessellator.startQuads();
  for(int cellX = cellMin; cellX < cellMax; ++cellX) {
   for(int cellZ = cellMin; cellZ < cellMax; ++cellZ) {
    if(!mask.opaque(texelOriginX + cellX, texelOriginZ + cellZ)) {
     continue;
    }
    const float baseX = static_cast<float>(cellX);
    const float baseZ = static_cast<float>(cellZ);
    const float drawX = baseX - fracX;
    const float drawZ = baseZ - fracZ;
    // The cell spans [drawX, drawX+1] x [cloudHeight, cloudHeight+thickness].
    if(cullCells && !cellFrustum.isVisible(static_cast<double>(drawX) - 1.0,
                                           static_cast<double>(cloudHeight) - 1.0,
                                           static_cast<double>(drawZ) - 1.0,
                                           static_cast<double>(drawX) + 2.0,
                                           static_cast<double>(cloudHeight + cloudThickness) + 1.0,
                                           static_cast<double>(drawZ) + 2.0)) {
     continue;
    }
    const float u0 = baseX * texScale + texOffsetX;
    const float u1 = (baseX + 1.0f) * texScale + texOffsetX;
    const float uMid = (baseX + 0.5f) * texScale + texOffsetX;
    const float v0 = baseZ * texScale + texOffsetZ;
    const float v1 = (baseZ + 1.0f) * texScale + texOffsetZ;
    const float vMid = (baseZ + 0.5f) * texScale + texOffsetZ;
    const float top = cloudHeight + cloudThickness;
    if(drawBottom) {
     tessellator.color(red * 0.7f, green * 0.7f, blue * 0.7f, 0.8f);
     tessellator.normal(0.0f, -1.0f, 0.0f);
     tessellator.vertex(drawX, cloudHeight, drawZ + 1.0f, u0, v1);
     tessellator.vertex(drawX + 1.0f, cloudHeight, drawZ + 1.0f, u1, v1);
     tessellator.vertex(drawX + 1.0f, cloudHeight, drawZ, u1, v0);
     tessellator.vertex(drawX, cloudHeight, drawZ, u0, v0);
    }
    if(drawTop) {
     tessellator.color(red, green, blue, 0.8f);
     tessellator.normal(0.0f, 1.0f, 0.0f);
     tessellator.vertex(drawX, top - edgeInset, drawZ + 1.0f, u0, v1);
     tessellator.vertex(drawX + 1.0f, top - edgeInset, drawZ + 1.0f, u1, v1);
     tessellator.vertex(drawX + 1.0f, top - edgeInset, drawZ, u1, v0);
     tessellator.vertex(drawX, top - edgeInset, drawZ, u0, v0);
    }
    tessellator.color(red * 0.9f, green * 0.9f, blue * 0.9f, 0.8f);
    if(!mask.opaque(texelOriginX + cellX - 1, texelOriginZ + cellZ)) {
     tessellator.normal(-1.0f, 0.0f, 0.0f);
     tessellator.vertex(drawX, cloudHeight, drawZ + 1.0f, uMid, v1);
     tessellator.vertex(drawX, top, drawZ + 1.0f, uMid, v1);
     tessellator.vertex(drawX, top, drawZ, uMid, v0);
     tessellator.vertex(drawX, cloudHeight, drawZ, uMid, v0);
    }
    if(!mask.opaque(texelOriginX + cellX + 1, texelOriginZ + cellZ)) {
     tessellator.normal(1.0f, 0.0f, 0.0f);
     tessellator.vertex(drawX + 1.0f - edgeInset, cloudHeight, drawZ + 1.0f, uMid, v1);
     tessellator.vertex(drawX + 1.0f - edgeInset, top, drawZ + 1.0f, uMid, v1);
     tessellator.vertex(drawX + 1.0f - edgeInset, top, drawZ, uMid, v0);
     tessellator.vertex(drawX + 1.0f - edgeInset, cloudHeight, drawZ, uMid, v0);
    }
    tessellator.color(red * 0.8f, green * 0.8f, blue * 0.8f, 0.8f);
    if(!mask.opaque(texelOriginX + cellX, texelOriginZ + cellZ - 1)) {
     tessellator.normal(0.0f, 0.0f, -1.0f);
     tessellator.vertex(drawX, top, drawZ, u0, vMid);
     tessellator.vertex(drawX + 1.0f, top, drawZ, u1, vMid);
     tessellator.vertex(drawX + 1.0f, cloudHeight, drawZ, u1, vMid);
     tessellator.vertex(drawX, cloudHeight, drawZ, u0, vMid);
    }
    if(!mask.opaque(texelOriginX + cellX, texelOriginZ + cellZ + 1)) {
     tessellator.normal(0.0f, 0.0f, 1.0f);
     tessellator.vertex(drawX, top, drawZ + 1.0f - edgeInset, u0, vMid);
     tessellator.vertex(drawX + 1.0f, top, drawZ + 1.0f - edgeInset, u1, vMid);
     tessellator.vertex(drawX + 1.0f, cloudHeight, drawZ + 1.0f - edgeInset, u1, vMid);
     tessellator.vertex(drawX, cloudHeight, drawZ + 1.0f - edgeInset, u0, vMid);
    }
   }
  }
  tessellator.draw();
 }
 core::disableBlend();
 core::enableCull();
}
} // namespace
void renderClouds(const AtmosphereContext& ctx, float tickDelta) {
 if(!ctx.settings.renderClouds) {
  return;
 }
 if(ctx.world == nullptr || ctx.world->dimension == nullptr || ctx.textureManager == nullptr ||
    ctx.camera == nullptr || ctx.world->dimension->isNether) {
  return;
 }
 const render::RenderPassScope pass(render::RenderType::clouds());
 const bool fancyClouds = ctx.settings.fancyClouds;
 if(fancyClouds) {
  renderFancyClouds(ctx, tickDelta);
 } else {
  core::disableCull();
  const float cameraY = static_cast<float>(ctx.camera->lastTickY + (ctx.camera->y - ctx.camera->lastTickY) *
                                                                       static_cast<double>(tickDelta));
  constexpr int tile = 8;
  constexpr int radius = 256 / tile;
  Tessellator& tessellator = Tessellator::INSTANCE;
  const int cloudTexture = ctx.textureManager->getTextureId("/environment/clouds.png");
  ctx.textureManager->bindTexture(cloudTexture);
  const CloudMask& mask = CloudMask::forTexture(*ctx.textureManager, cloudTexture);
  Vec3d cloudColor = cloudColorForWorld(ctx.world, tickDelta);
  float red = static_cast<float>(cloudColor.x);
  float green = static_cast<float>(cloudColor.y);
  float blue = static_cast<float>(cloudColor.z);
  constexpr float scrollScale = 4.8828125E-4f;
  double cloudX = ctx.camera->prevX + (ctx.camera->x - ctx.camera->prevX) * static_cast<double>(tickDelta) +
                  static_cast<double>((static_cast<float>(ctx.atmosphereTicks) + tickDelta) * 0.03f);
  double cloudZ = ctx.camera->prevZ + (ctx.camera->z - ctx.camera->prevZ) * static_cast<double>(tickDelta);
  const int originX = MathHelper::floor(cloudX / 2048.0);
  const int originZ = MathHelper::floor(cloudZ / 2048.0);
  const float cloudHeight = client::option::cloudHeightOffset(
      ctx.world->dimension->getCloudHeight() - cameraY + 0.33f, ctx.settings);
  cloudX -= static_cast<double>(originX * 2048);
  cloudZ -= static_cast<double>(originZ * 2048);
  const int texelOriginX = MathHelper::floor(cloudX / static_cast<double>(tile));
  const int texelOriginZ = MathHelper::floor(cloudZ / static_cast<double>(tile));
  const float texOffsetX = static_cast<float>(texelOriginX * tile) * scrollScale;
  const float texOffsetZ = static_cast<float>(texelOriginZ * tile) * scrollScale;
  const float fracX = static_cast<float>(cloudX - static_cast<double>(texelOriginX * tile));
  const float fracZ = static_cast<float>(cloudZ - static_cast<double>(texelOriginZ * tile));
  // The grid is 64x64 cells and was rebuilt whole every frame -- up to 16k
  // vertices, three quarters of them behind the camera.
  //
  // Tessellator bakes core::drawPose() into each vertex on the CPU and the pass
  // then draws with drawModelView()/drawProjection(). Folding the pose into the
  // frustum puts its planes in the same pre-pose space as the cell coordinates
  // below, so the test is right whether or not a pose is active (it is identity
  // here, but renderFancyClouds sets a scale and this must not depend on that).
  Frustum cellFrustum;
  const bool cullCells = core::drawCameraStateValid();
  if(cullCells) {
   net::minecraft::util::math::Matrix4f posedModelView = core::drawModelView();
   posedModelView.multiply(core::drawPose());
   cellFrustum.compute(core::drawProjection(), posedModelView, 0.0, 0.0, 0.0);
  }
  constexpr float cellMargin = 1.0f;
  tessellator.startQuads();
  tessellator.color(red, green, blue, 0.8f);
  for(int cellX = -radius; cellX < radius; ++cellX) {
   for(int cellZ = -radius; cellZ < radius; ++cellZ) {
    if(!mask.opaque(texelOriginX + cellX, texelOriginZ + cellZ)) {
     continue;
    }
    const int baseX = cellX * tile;
    const int baseZ = cellZ * tile;
    const float drawX = static_cast<float>(baseX) - fracX;
    const float drawZ = static_cast<float>(baseZ) - fracZ;
    if(cullCells && !cellFrustum.isVisible(static_cast<double>(drawX - cellMargin),
                                           static_cast<double>(cloudHeight - cellMargin),
                                           static_cast<double>(drawZ - cellMargin),
                                           static_cast<double>(drawX + static_cast<float>(tile) + cellMargin),
                                           static_cast<double>(cloudHeight + cellMargin),
                                           static_cast<double>(drawZ + static_cast<float>(tile) + cellMargin))) {
     continue;
    }
    const float u0 = static_cast<float>(baseX) * scrollScale + texOffsetX;
    const float u1 = static_cast<float>(baseX + tile) * scrollScale + texOffsetX;
    const float v0 = static_cast<float>(baseZ) * scrollScale + texOffsetZ;
    const float v1 = static_cast<float>(baseZ + tile) * scrollScale + texOffsetZ;
    tessellator.vertex(drawX, cloudHeight, drawZ + static_cast<float>(tile), u0, v1);
    tessellator.vertex(drawX + static_cast<float>(tile), cloudHeight, drawZ + static_cast<float>(tile), u1, v1);
    tessellator.vertex(drawX + static_cast<float>(tile), cloudHeight, drawZ, u1, v0);
    tessellator.vertex(drawX, cloudHeight, drawZ, u0, v0);
   }
  }
  tessellator.draw();
 }
 core::disableBlend();
 core::enableCull();
}
} // namespace net::minecraft::client::render::atmosphere
