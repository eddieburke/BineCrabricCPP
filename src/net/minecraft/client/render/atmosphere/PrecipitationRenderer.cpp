#include "net/minecraft/client/render/atmosphere/PrecipitationRenderer.hpp"
#include <vector>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereContext.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
namespace net::minecraft::client::render::atmosphere {
void PrecipitationRenderer::renderPrecipitation(const AtmosphereContext& ctx, float tickDelta) {
 if(ctx.world == nullptr || ctx.camera == nullptr) {
  return;
 }
 const float rain = ctx.client != nullptr
                        ? client::option::rainGradient(client::option::resolve(ctx.options), ctx.world, tickDelta)
                        : ctx.world->getRainGradient(tickDelta);
 if(rain <= 0.0f) {
  return;
 }
 if(ctx.livingCamera == nullptr) {
  return;
 }
 net::minecraft::World* world = ctx.world;
 BiomeSource* biomeSource = world->getBiomeSource();
 if(biomeSource == nullptr) {
  return;
 }
 const int centerX = MathHelper::floor(ctx.livingCamera->x);
 const int centerY = MathHelper::floor(ctx.livingCamera->y);
 const int centerZ = MathHelper::floor(ctx.livingCamera->z);
 const double interpX = ctx.livingCamera->lastTickX +
                        (ctx.livingCamera->x - ctx.livingCamera->lastTickX) * static_cast<double>(tickDelta);
 const double interpY = ctx.livingCamera->lastTickY +
                        (ctx.livingCamera->y - ctx.livingCamera->lastTickY) * static_cast<double>(tickDelta);
 const double interpZ = ctx.livingCamera->lastTickZ +
                        (ctx.livingCamera->z - ctx.livingCamera->lastTickZ) * static_cast<double>(tickDelta);
 Tessellator& tessellator = INSTANCE;
 render::RenderPassScope passScope(render::RenderType::guiTextured());
 render::RenderSystem::alphaTest(0.01f);
 const int floorY = MathHelper::floor(interpY);
 int radius = client::option::resolve(ctx.options).fancyPrecipitation ? 10 : 5;
 std::vector<Biome*> biomeScratch;
 biomeSource->getBiomesInArea(biomeScratch, centerX - radius, centerZ - radius, radius * 2 + 1, radius * 2 + 1);
 render::RenderSystem::bindTexture(ctx.textureManager->getTextureId("/environment/snow.png"));
 int biomeIndex = 0;
 tessellator.startQuads();
 tessellator.translate(-interpX, -interpY, -interpZ);
 for(int x = centerX - radius; x <= centerX + radius; ++x) {
  for(int z = centerZ - radius; z <= centerZ + radius; ++z) {
   Biome* biome = biomeScratch[static_cast<std::size_t>(biomeIndex++)];
   if(biome == nullptr || !biome->canSnow()) {
    continue;
   }
   int top = world->getTopSolidBlockY(x, z);
   if(top < 0) {
    top = 0;
   }
   int low = top;
   if(low < floorY) {
    low = floorY;
   }
   int high = centerY - radius;
   const int highCap = centerY + radius;
   if(high < top) {
    high = top;
   }
   if(highCap < top) {
    continue;
   }
   if(high == highCap) {
    continue;
   }
   random_.setSeed(static_cast<std::uint64_t>(x * x * 3121 + x * 45238971 + z * z * 418711 + z * 13761));
   const float phase = static_cast<float>(ctx.atmosphereTicks) + tickDelta;
   const float scroll = (static_cast<float>(ctx.atmosphereTicks & 0x1FF) + tickDelta) / 512.0f;
   const float uOff = random_.nextFloat() + phase * 0.01f * static_cast<float>(random_.nextGaussian());
   const float vOff = random_.nextFloat() + phase * static_cast<float>(random_.nextGaussian()) * 0.001f;
   const double dx = static_cast<double>(static_cast<float>(x) + 0.5f) - ctx.livingCamera->x;
   const double dz = static_cast<double>(static_cast<float>(z) + 0.5f) - ctx.livingCamera->z;
   const float dist = MathHelper::sqrt(static_cast<float>(dx * dx + dz * dz)) / static_cast<float>(radius);
   const float brightness = world->getLightBrightness(x, low, z);
   render::RenderSystem::color4f(brightness, brightness, brightness, ((1.0f - dist * dist) * 0.3f + 0.5f) * rain);
   constexpr float texScale = 1.0f;
   tessellator.vertex(x + 0,
                      high,
                      static_cast<double>(z) + 0.5,
                      0.0f * texScale + uOff,
                      static_cast<float>(high) * texScale / 4.0f + scroll * texScale + vOff);
   tessellator.vertex(x + 1,
                      high,
                      static_cast<double>(z) + 0.5,
                      1.0f * texScale + uOff,
                      static_cast<float>(high) * texScale / 4.0f + scroll * texScale + vOff);
   tessellator.vertex(x + 1,
                      highCap,
                      static_cast<double>(z) + 0.5,
                      1.0f * texScale + uOff,
                      static_cast<float>(highCap) * texScale / 4.0f + scroll * texScale + vOff);
   tessellator.vertex(x + 0,
                      highCap,
                      static_cast<double>(z) + 0.5,
                      0.0f * texScale + uOff,
                      static_cast<float>(highCap) * texScale / 4.0f + scroll * texScale + vOff);
   tessellator.vertex(static_cast<double>(x) + 0.5,
                      high,
                      z + 0,
                      0.0f * texScale + uOff,
                      static_cast<float>(high) * texScale / 4.0f + scroll * texScale + vOff);
   tessellator.vertex(static_cast<double>(x) + 0.5,
                      high,
                      z + 1,
                      1.0f * texScale + uOff,
                      static_cast<float>(high) * texScale / 4.0f + scroll * texScale + vOff);
   tessellator.vertex(static_cast<double>(x) + 0.5,
                      highCap,
                      z + 1,
                      1.0f * texScale + uOff,
                      static_cast<float>(highCap) * texScale / 4.0f + scroll * texScale + vOff);
   tessellator.vertex(static_cast<double>(x) + 0.5,
                      highCap,
                      z + 0,
                      0.0f * texScale + uOff,
                      static_cast<float>(highCap) * texScale / 4.0f + scroll * texScale + vOff);
  }
 }
 tessellator.draw();
 tessellator.translate(0.0, 0.0, 0.0);
 render::RenderSystem::bindTexture(ctx.textureManager->getTextureId("/environment/rain.png"));
 if(client::option::resolve(ctx.options).fancyPrecipitation) {
  radius = 10;
 }
 biomeIndex = 0;
 tessellator.startQuads();
 tessellator.translate(-interpX, -interpY, -interpZ);
 for(int x = centerX - radius; x <= centerX + radius; ++x) {
  for(int z = centerZ - radius; z <= centerZ + radius; ++z) {
   Biome* biome = biomeScratch[static_cast<std::size_t>(biomeIndex++)];
   if(biome == nullptr || !biome->canRain()) {
    continue;
   }
   const int top = world->getTopSolidBlockY(x, z);
   int low = centerY - radius;
   int high = centerY + radius;
   if(low < top) {
    low = top;
   }
   if(high < top) {
    high = top;
   }
   if(low == high) {
    continue;
   }
   random_.setSeed(static_cast<std::uint64_t>(x * x * 3121 + x * 45238971 + z * z * 418711 + z * 13761));
   const float anim =
       (static_cast<float>((ctx.atmosphereTicks + x * x * 3121 + x * 45238971 + z * z * 418711 + z * 13761) &
                           0x1F) +
        tickDelta) /
       32.0f * (3.0f + random_.nextFloat());
   const double dx = static_cast<double>(static_cast<float>(x) + 0.5f) - ctx.livingCamera->x;
   const double dz = static_cast<double>(static_cast<float>(z) + 0.5f) - ctx.livingCamera->z;
   const float dist = MathHelper::sqrt(static_cast<float>(dx * dx + dz * dz)) / static_cast<float>(radius);
   const float brightness = world->getLightBrightness(x, 128, z) * 0.85f + 0.15f;
   render::RenderSystem::color4f(brightness, brightness, brightness, ((1.0f - dist * dist) * 0.5f + 0.5f) * rain);
   constexpr float texScale = 1.0f;
   tessellator.vertex(x + 0,
                      low,
                      static_cast<double>(z) + 0.5,
                      0.0f * texScale,
                      static_cast<float>(low) * texScale / 4.0f + anim * texScale);
   tessellator.vertex(x + 1,
                      low,
                      static_cast<double>(z) + 0.5,
                      1.0f * texScale,
                      static_cast<float>(low) * texScale / 4.0f + anim * texScale);
   tessellator.vertex(x + 1,
                      high,
                      static_cast<double>(z) + 0.5,
                      1.0f * texScale,
                      static_cast<float>(high) * texScale / 4.0f + anim * texScale);
   tessellator.vertex(x + 0,
                      high,
                      static_cast<double>(z) + 0.5,
                      0.0f * texScale,
                      static_cast<float>(high) * texScale / 4.0f + anim * texScale);
   tessellator.vertex(static_cast<double>(x) + 0.5,
                      low,
                      z + 0,
                      0.0f * texScale,
                      static_cast<float>(low) * texScale / 4.0f + anim * texScale);
   tessellator.vertex(static_cast<double>(x) + 0.5,
                      low,
                      z + 1,
                      1.0f * texScale,
                      static_cast<float>(low) * texScale / 4.0f + anim * texScale);
   tessellator.vertex(static_cast<double>(x) + 0.5,
                      high,
                      z + 1,
                      1.0f * texScale,
                      static_cast<float>(high) * texScale / 4.0f + anim * texScale);
   tessellator.vertex(static_cast<double>(x) + 0.5,
                      high,
                      z + 0,
                      0.0f * texScale,
                      static_cast<float>(high) * texScale / 4.0f + anim * texScale);
  }
 }
 tessellator.draw();
 tessellator.translate(0.0, 0.0, 0.0);
}
} // namespace net::minecraft::client::render::atmosphere
