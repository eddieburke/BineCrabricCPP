#include "net/minecraft/client/render/atmosphere/CloudRenderer.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereContext.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
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
} // namespace
void CloudRenderer::renderFancyClouds(const AtmosphereContext& ctx, float tickDelta) {
  if(ctx.camera == nullptr) {
    return;
  }
  gl::GL11::glDisable(gl::GL11::GL_CULL_FACE);
  const float cameraY = static_cast<float>(ctx.camera->lastTickY +
                                           (ctx.camera->y - ctx.camera->lastTickY) * static_cast<double>(tickDelta));
  Tessellator& tessellator = INSTANCE;
  constexpr float cloudScale = 12.0f;
  constexpr float cloudThickness = 4.0f;
  double cloudX = (ctx.camera->prevX + (ctx.camera->x - ctx.camera->prevX) * static_cast<double>(tickDelta) +
                   static_cast<double>((static_cast<float>(ctx.atmosphereTicks) + tickDelta) * 0.03f)) /
                  static_cast<double>(cloudScale);
  double cloudZ = (ctx.camera->prevZ + (ctx.camera->z - ctx.camera->prevZ) * static_cast<double>(tickDelta)) /
                      static_cast<double>(cloudScale) +
                  0.33;
  const float cloudHeight = client::option::cloudHeightOffset(
      ctx.world->dimension->getCloudHeight() - cameraY + 0.33f, client::option::resolve(ctx.options));
  const int originX = MathHelper::floor(cloudX / 2048.0);
  const int originZ = MathHelper::floor(cloudZ / 2048.0);
  cloudX -= static_cast<double>(originX * 2048);
  cloudZ -= static_cast<double>(originZ * 2048);
  gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, ctx.textureManager->getTextureId("/environment/clouds.png"));
  gl::GL11::glEnable(gl::GL11::GL_BLEND);
  gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
  Vec3d cloudColor = cloudColorForWorld(ctx.world, tickDelta);
  float red = static_cast<float>(cloudColor.x);
  float green = static_cast<float>(cloudColor.y);
  float blue = static_cast<float>(cloudColor.z);
  float texOffsetX = static_cast<float>(MathHelper::floor(cloudX) * 0.00390625f);
  float texOffsetZ = static_cast<float>(MathHelper::floor(cloudZ) * 0.00390625f);
  const float fracX = static_cast<float>(cloudX - static_cast<double>(MathHelper::floor(cloudX)));
  const float fracZ = static_cast<float>(cloudZ - static_cast<double>(MathHelper::floor(cloudZ)));
  constexpr float texScale = 0.00390625f;
  constexpr float edgeInset = 9.765625E-4f;
  constexpr int tileSize = 8;
  constexpr int tileRadius = 3;
  gl::GL11::glPushMatrix();
  gl::GL11::glScalef(cloudScale, 1.0f, cloudScale);
  for(int pass = 0; pass < 2; ++pass) {
    if(pass == 0) {
      gl::GL11::glColorMask(false, false, false, false);
    } else {
      gl::GL11::glColorMask(true, true, true, true);
    }
    for(int tileX = -tileRadius + 1; tileX <= tileRadius; ++tileX) {
      for(int tileZ = -tileRadius + 1; tileZ <= tileRadius; ++tileZ) {
        const float baseX = static_cast<float>(tileX * tileSize);
        const float baseZ = static_cast<float>(tileZ * tileSize);
        const float drawX = baseX - fracX;
        const float drawZ = baseZ - fracZ;
        tessellator.startQuads();
        if(cloudHeight > -cloudThickness - 1.0f) {
          tessellator.color(red * 0.7f, green * 0.7f, blue * 0.7f, 0.8f);
          tessellator.normal(0.0f, -1.0f, 0.0f);
          tessellator.vertex(drawX, cloudHeight, drawZ + static_cast<float>(tileSize),
                             (baseX + 0.0f) * texScale + texOffsetX,
                             (baseZ + static_cast<float>(tileSize)) * texScale + texOffsetZ);
          tessellator.vertex(drawX + static_cast<float>(tileSize), cloudHeight,
                             drawZ + static_cast<float>(tileSize),
                             (baseX + static_cast<float>(tileSize)) * texScale + texOffsetX,
                             (baseZ + static_cast<float>(tileSize)) * texScale + texOffsetZ);
          tessellator.vertex(drawX + static_cast<float>(tileSize), cloudHeight, drawZ,
                             (baseX + static_cast<float>(tileSize)) * texScale + texOffsetX,
                             (baseZ + 0.0f) * texScale + texOffsetZ);
          tessellator.vertex(drawX, cloudHeight, drawZ, (baseX + 0.0f) * texScale + texOffsetX,
                             (baseZ + 0.0f) * texScale + texOffsetZ);
        }
        if(cloudHeight <= cloudThickness + 1.0f) {
          tessellator.color(red, green, blue, 0.8f);
          tessellator.normal(0.0f, 1.0f, 0.0f);
          tessellator.vertex(drawX, cloudHeight + cloudThickness - edgeInset,
                             drawZ + static_cast<float>(tileSize), (baseX + 0.0f) * texScale + texOffsetX,
                             (baseZ + static_cast<float>(tileSize)) * texScale + texOffsetZ);
          tessellator.vertex(drawX + static_cast<float>(tileSize), cloudHeight + cloudThickness - edgeInset,
                             drawZ + static_cast<float>(tileSize),
                             (baseX + static_cast<float>(tileSize)) * texScale + texOffsetX,
                             (baseZ + static_cast<float>(tileSize)) * texScale + texOffsetZ);
          tessellator.vertex(drawX + static_cast<float>(tileSize), cloudHeight + cloudThickness - edgeInset,
                             drawZ, (baseX + static_cast<float>(tileSize)) * texScale + texOffsetX,
                             (baseZ + 0.0f) * texScale + texOffsetZ);
          tessellator.vertex(drawX, cloudHeight + cloudThickness - edgeInset, drawZ,
                             (baseX + 0.0f) * texScale + texOffsetX, (baseZ + 0.0f) * texScale + texOffsetZ);
        }
        tessellator.color(red * 0.9f, green * 0.9f, blue * 0.9f, 0.8f);
        if(tileX > -1) {
          tessellator.normal(-1.0f, 0.0f, 0.0f);
          for(int segment = 0; segment < tileSize; ++segment) {
            tessellator.vertex(drawX + static_cast<float>(segment), cloudHeight,
                               drawZ + static_cast<float>(tileSize),
                               (baseX + static_cast<float>(segment) + 0.5f) * texScale + texOffsetX,
                               (baseZ + static_cast<float>(tileSize)) * texScale + texOffsetZ);
            tessellator.vertex(drawX + static_cast<float>(segment), cloudHeight + cloudThickness,
                               drawZ + static_cast<float>(tileSize),
                               (baseX + static_cast<float>(segment) + 0.5f) * texScale + texOffsetX,
                               (baseZ + static_cast<float>(tileSize)) * texScale + texOffsetZ);
            tessellator.vertex(drawX + static_cast<float>(segment), cloudHeight + cloudThickness, drawZ,
                               (baseX + static_cast<float>(segment) + 0.5f) * texScale + texOffsetX,
                               (baseZ + 0.0f) * texScale + texOffsetZ);
            tessellator.vertex(drawX + static_cast<float>(segment), cloudHeight, drawZ,
                               (baseX + static_cast<float>(segment) + 0.5f) * texScale + texOffsetX,
                               (baseZ + 0.0f) * texScale + texOffsetZ);
          }
        }
        if(tileX <= 1) {
          tessellator.normal(1.0f, 0.0f, 0.0f);
          for(int segment = 0; segment < tileSize; ++segment) {
            tessellator.vertex(drawX + static_cast<float>(segment) + 1.0f - edgeInset, cloudHeight,
                               drawZ + static_cast<float>(tileSize),
                               (baseX + static_cast<float>(segment) + 0.5f) * texScale + texOffsetX,
                               (baseZ + static_cast<float>(tileSize)) * texScale + texOffsetZ);
            tessellator.vertex(drawX + static_cast<float>(segment) + 1.0f - edgeInset,
                               cloudHeight + cloudThickness, drawZ + static_cast<float>(tileSize),
                               (baseX + static_cast<float>(segment) + 0.5f) * texScale + texOffsetX,
                               (baseZ + static_cast<float>(tileSize)) * texScale + texOffsetZ);
            tessellator.vertex(drawX + static_cast<float>(segment) + 1.0f - edgeInset,
                               cloudHeight + cloudThickness, drawZ,
                               (baseX + static_cast<float>(segment) + 0.5f) * texScale + texOffsetX,
                               (baseZ + 0.0f) * texScale + texOffsetZ);
            tessellator.vertex(drawX + static_cast<float>(segment) + 1.0f - edgeInset, cloudHeight, drawZ,
                               (baseX + static_cast<float>(segment) + 0.5f) * texScale + texOffsetX,
                               (baseZ + 0.0f) * texScale + texOffsetZ);
          }
        }
        tessellator.color(red * 0.8f, green * 0.8f, blue * 0.8f, 0.8f);
        if(tileZ > -1) {
          tessellator.normal(0.0f, 0.0f, -1.0f);
          for(int segment = 0; segment < tileSize; ++segment) {
            tessellator.vertex(drawX, cloudHeight + cloudThickness, drawZ + static_cast<float>(segment),
                               (baseX + 0.0f) * texScale + texOffsetX,
                               (baseZ + static_cast<float>(segment) + 0.5f) * texScale + texOffsetZ);
            tessellator.vertex(drawX + static_cast<float>(tileSize), cloudHeight + cloudThickness,
                               drawZ + static_cast<float>(segment),
                               (baseX + static_cast<float>(tileSize)) * texScale + texOffsetX,
                               (baseZ + static_cast<float>(segment) + 0.5f) * texScale + texOffsetZ);
            tessellator.vertex(drawX + static_cast<float>(tileSize), cloudHeight,
                               drawZ + static_cast<float>(segment),
                               (baseX + static_cast<float>(tileSize)) * texScale + texOffsetX,
                               (baseZ + static_cast<float>(segment) + 0.5f) * texScale + texOffsetZ);
            tessellator.vertex(drawX, cloudHeight, drawZ + static_cast<float>(segment),
                               (baseX + 0.0f) * texScale + texOffsetX,
                               (baseZ + static_cast<float>(segment) + 0.5f) * texScale + texOffsetZ);
          }
        }
        if(tileZ <= 1) {
          tessellator.normal(0.0f, 0.0f, 1.0f);
          for(int segment = 0; segment < tileSize; ++segment) {
            tessellator.vertex(drawX, cloudHeight + cloudThickness,
                               drawZ + static_cast<float>(segment) + 1.0f - edgeInset,
                               (baseX + 0.0f) * texScale + texOffsetX,
                               (baseZ + static_cast<float>(segment) + 0.5f) * texScale + texOffsetZ);
            tessellator.vertex(drawX + static_cast<float>(tileSize), cloudHeight + cloudThickness,
                               drawZ + static_cast<float>(segment) + 1.0f - edgeInset,
                               (baseX + static_cast<float>(tileSize)) * texScale + texOffsetX,
                               (baseZ + static_cast<float>(segment) + 0.5f) * texScale + texOffsetZ);
            tessellator.vertex(drawX + static_cast<float>(tileSize), cloudHeight,
                               drawZ + static_cast<float>(segment) + 1.0f - edgeInset,
                               (baseX + static_cast<float>(tileSize)) * texScale + texOffsetX,
                               (baseZ + static_cast<float>(segment) + 0.5f) * texScale + texOffsetZ);
            tessellator.vertex(drawX, cloudHeight, drawZ + static_cast<float>(segment) + 1.0f - edgeInset,
                               (baseX + 0.0f) * texScale + texOffsetX,
                               (baseZ + static_cast<float>(segment) + 0.5f) * texScale + texOffsetZ);
          }
        }
        tessellator.draw();
      }
    }
  }
  gl::GL11::glPopMatrix();
  gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  gl::GL11::glDisable(gl::GL11::GL_BLEND);
  gl::GL11::glEnable(gl::GL11::GL_CULL_FACE);
}
void CloudRenderer::renderClouds(const AtmosphereContext& ctx, float tickDelta) {
  if(!client::option::resolve(ctx.options).renderClouds) {
    return;
  }
  if(ctx.world == nullptr || ctx.world->dimension == nullptr || ctx.textureManager == nullptr ||
     ctx.camera == nullptr || ctx.world->dimension->isNether) {
    return;
  }
  const gl::AttribGuard state(gl::GL11::GL_ENABLE_BIT | gl::GL11::GL_COLOR_BUFFER_BIT |
                              gl::GL11::GL_DEPTH_BUFFER_BIT | gl::GL11::GL_TEXTURE_BIT |
                              gl::GL11::GL_CURRENT_BIT);
  gl::GL11::glEnable(gl::GL11::GL_DEPTH_TEST);
  gl::GL11::glDepthFunc(gl::GL11::GL_LEQUAL);
  gl::GL11::glDepthMask(true);
  gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
  gl::GL11::glColorMask(true, true, true, true);
  gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  const bool fancyClouds = client::option::resolve(ctx.options).fancyClouds;
  if(fancyClouds) {
    renderFancyClouds(ctx, tickDelta);
  } else {
    gl::GL11::glDisable(gl::GL11::GL_CULL_FACE);
    const float cameraY = static_cast<float>(ctx.camera->lastTickY +
                                             (ctx.camera->y - ctx.camera->lastTickY) * static_cast<double>(tickDelta));
    constexpr int tile = 32;
    constexpr int radius = 256 / tile;
    Tessellator& tessellator = INSTANCE;
    gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, ctx.textureManager->getTextureId("/environment/clouds.png"));
    gl::GL11::glEnable(gl::GL11::GL_BLEND);
    gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
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
        ctx.world->dimension->getCloudHeight() - cameraY + 0.33f, client::option::resolve(ctx.options));
    const float texOffsetX =
        static_cast<float>((cloudX -= static_cast<double>(originX * 2048)) * static_cast<double>(scrollScale));
    const float texOffsetZ =
        static_cast<float>((cloudZ -= static_cast<double>(originZ * 2048)) * static_cast<double>(scrollScale));
    tessellator.startQuads();
    tessellator.color(red, green, blue, 0.8f);
    for(int x = -tile * radius; x < tile * radius; x += tile) {
      for(int z = -tile * radius; z < tile * radius; z += tile) {
        tessellator.vertex(x + 0, cloudHeight, z + tile, static_cast<float>(x + 0) * scrollScale + texOffsetX,
                           static_cast<float>(z + tile) * scrollScale + texOffsetZ);
        tessellator.vertex(x + tile, cloudHeight, z + tile, static_cast<float>(x + tile) * scrollScale + texOffsetX,
                           static_cast<float>(z + tile) * scrollScale + texOffsetZ);
        tessellator.vertex(x + tile, cloudHeight, z + 0, static_cast<float>(x + tile) * scrollScale + texOffsetX,
                           static_cast<float>(z + 0) * scrollScale + texOffsetZ);
        tessellator.vertex(x + 0, cloudHeight, z + 0, static_cast<float>(x + 0) * scrollScale + texOffsetX,
                           static_cast<float>(z + 0) * scrollScale + texOffsetZ);
      }
    }
    tessellator.draw();
  }
  gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  gl::GL11::glDisable(gl::GL11::GL_BLEND);
  gl::GL11::glEnable(gl::GL11::GL_CULL_FACE);
}
} // namespace net::minecraft::client::render::atmosphere
