#include "net/minecraft/client/render/atmosphere/SkyDome.hpp"
#include <array>
#include <cmath>
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereContext.hpp"
#include "net/minecraft/client/gl/Lighting.hpp"
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/mod/GameHooks.hpp"
#include "net/minecraft/mod/HookBus.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
namespace net::minecraft::client::render::atmosphere {
namespace {
constexpr float kPi = 3.14159265358979323846f;
struct SkyMeshes {
  TessellatorMesh lightSky;
  TessellatorMesh darkSky;
  TessellatorMesh stars;
  bool built = false;
};
SkyMeshes& skyMeshes() {
  static SkyMeshes meshes;
  return meshes;
}
void buildStarMesh(Tessellator& tessellator) {
  net::minecraft::JavaRandom random(10842ULL);
  tessellator.start(gl::prim::Quads);
  for(int i = 0; i < 1500; ++i) {
    double d = static_cast<double>(random.nextFloat() * 2.0f - 1.0f);
    double d2 = static_cast<double>(random.nextFloat() * 2.0f - 1.0f);
    double d3 = static_cast<double>(random.nextFloat() * 2.0f - 1.0f);
    const double d4 = static_cast<double>(0.25f + random.nextFloat() * 0.25f);
    const double d5 = d * d + d2 * d2 + d3 * d3;
    if(!(d5 < 1.0) || !(d5 > 0.01)) {
      continue;
    }
    const double inv = 1.0 / std::sqrt(d5);
    d *= inv;
    d2 *= inv;
    d3 *= inv;
    const double d6 = d * 100.0;
    const double d7 = d2 * 100.0;
    const double d8 = d3 * 100.0;
    const double d9 = std::atan2(d, d3);
    const double d10 = std::sin(d9);
    const double d11 = std::cos(d9);
    const double d12 = std::atan2(std::sqrt(d * d + d3 * d3), d2);
    const double d13 = std::sin(d12);
    const double d14 = std::cos(d12);
    const double d15 = random.nextDouble() * 3.141592653589793 * 2.0;
    const double d16 = std::sin(d15);
    const double d17 = std::cos(d15);
    for(int j = 0; j < 4; ++j) {
      const double d20 = static_cast<double>((j & 2) - 1) * d4;
      const double d21 = static_cast<double>(((j + 1) & 2) - 1) * d4;
      const double d23 = d20 * d17 - d21 * d16;
      const double d24 = d21 * d17 + d20 * d16;
      const double d25 = d23 * d13;
      const double d26 = -d23 * d14;
      const double d27 = d26 * d10 - d24 * d11;
      const double d28 = d25;
      const double d29 = d24 * d10 + d26 * d11;
      tessellator.vertex(d6 + d27, d7 + d28, d8 + d29);
    }
  }
}
void buildSkyDomes(SkyMeshes& meshes) {
  Tessellator& tessellator = Tessellator::INSTANCE;
  tessellator.setCaptureOnly(true);
  buildStarMesh(tessellator);
  meshes.stars = tessellator.takeMesh();
  (void)meshes.stars.uploadToGpu();
  constexpr int step = 64;
  constexpr int span = 4096 / step;
  tessellator.start(gl::prim::Quads);
  for(int x = -step * span; x <= step * span; x += step) {
    for(int z = -step * span; z <= step * span; z += step) {
      tessellator.vertex(x + 0, 16.0f, z + 0);
      tessellator.vertex(x + step, 16.0f, z + 0);
      tessellator.vertex(x + step, 16.0f, z + step);
      tessellator.vertex(x + 0, 16.0f, z + step);
    }
  }
  meshes.lightSky = tessellator.takeMesh();
  (void)meshes.lightSky.uploadToGpu();
  tessellator.start(gl::prim::Quads);
  for(int x = -step * span; x <= step * span; x += step) {
    for(int z = -step * span; z <= step * span; z += step) {
      tessellator.vertex(x + step, -16.0f, z + 0);
      tessellator.vertex(x + 0, -16.0f, z + 0);
      tessellator.vertex(x + 0, -16.0f, z + step);
      tessellator.vertex(x + step, -16.0f, z + step);
    }
  }
  meshes.darkSky = tessellator.takeMesh();
  (void)meshes.darkSky.uploadToGpu();
  tessellator.setCaptureOnly(false);
  meshes.built = true;
}
void publishRenderStage(mod::WorldRenderEvent& event, mod::WorldRenderStage stage, mod::RenderHookMoment moment) {
  event.stage = stage;
  event.moment = moment;
  mod::hooks().publish(event);
}
void drawBackgroundFan(const AtmosphereContext& ctx, float tickDelta, const std::array<float, 4>& bg) {
  const float timeOfDay = ctx.world->getTime(tickDelta);
  const gl::ShadeModelScope shadeCaps;
  gl::shadeModel(gl::shade::Smooth);
  gl::MatrixGuard fanMatrix;
  // Fan lies in the XZ plane (horizontal) at Y=0. Center at Z=+100 for
  // dusk (sun in front) or Z=-100 for dawn (sun behind) via the X-axis flip.
  gl::rotatef(timeOfDay > 0.5f ? 270.0f : 90.0f, 1.0f, 0.0f, 0.0f);
  Tessellator& tessellator = Tessellator::INSTANCE;
  tessellator.start(gl::prim::TriangleFan);
  tessellator.color(bg[0], bg[1], bg[2], bg[3]);
  tessellator.vertex(0.0, 100.0, 0.0);
  tessellator.color(bg[0], bg[1], bg[2], 0.0f);
  for(int i = 0; i <= 16; ++i) {
    const float angle = static_cast<float>(i) * kPi * 2.0f / 16.0f;
    tessellator.vertex(std::sin(angle) * 120.0, std::cos(angle) * 120.0, 0.0);
  }
  tessellator.draw();
}
void drawSunMoon(const AtmosphereContext& ctx, float starAlpha) {
  gl::blendFunc(gl::blend::SrcAlpha, gl::blend::One);
  gl::color4f(1.0f, 1.0f, 1.0f, starAlpha);
  Tessellator& tessellator = Tessellator::INSTANCE;
  if(ctx.textureManager != nullptr) {
    gl::bindTexture(gl::cap::Texture2D, ctx.textureManager->getTextureId("/terrain/sun.png"));
  }
  tessellator.startQuads();
  tessellator.vertex(-30.0, 100.0, -30.0, 0.0, 0.0);
  tessellator.vertex(30.0, 100.0, -30.0, 1.0, 0.0);
  tessellator.vertex(30.0, 100.0, 30.0, 1.0, 1.0);
  tessellator.vertex(-30.0, 100.0, 30.0, 0.0, 1.0);
  tessellator.draw();
  if(ctx.textureManager != nullptr) {
    gl::bindTexture(gl::cap::Texture2D, ctx.textureManager->getTextureId("/terrain/moon.png"));
  }
  tessellator.startQuads();
  tessellator.vertex(-20.0, -100.0, 20.0, 1.0, 1.0);
  tessellator.vertex(20.0, -100.0, 20.0, 0.0, 1.0);
  tessellator.vertex(20.0, -100.0, -20.0, 0.0, 0.0);
  tessellator.vertex(-20.0, -100.0, -20.0, 1.0, 0.0);
  tessellator.draw();
}
} // namespace
void renderSkyDome(const AtmosphereContext& ctx, float tickDelta) {
  if(ctx.world == nullptr || ctx.world->dimension == nullptr || ctx.camera == nullptr ||
     ctx.world->dimension->isNether) {
    return;
  }
  const float timeOfDay = ctx.world->getTime(tickDelta);
  mod::WorldRenderEvent skyEvent{
      ctx.world,
      ctx.camera,
      tickDelta,
      mod::WorldRenderStage::Sky,
      mod::RenderHookMoment::Before,
      false,
      false,
      timeOfDay * kPi * 2.0f,
      0.0f,
  };
  publishRenderStage(skyEvent, mod::WorldRenderStage::Sky, mod::RenderHookMoment::Before);
  {
    float sunX = skyEvent.sunDirectionX;
    float sunY = skyEvent.sunDirectionY;
    float sunZ = skyEvent.sunDirectionZ;
    if(!skyEvent.solarDirectionValid) {
      const float yaw = skyEvent.skyYawDegrees * kPi / 180.0f;
      sunX = std::sin(yaw) * std::sin(skyEvent.celestialAngle);
      sunY = std::cos(skyEvent.celestialAngle);
      sunZ = std::cos(yaw) * std::sin(skyEvent.celestialAngle);
    }
    const float length = std::sqrt(sunX * sunX + sunY * sunY + sunZ * sunZ);
    if(length > 0.0001f) {
      sunX /= length;
      sunY /= length;
      sunZ /= length;
    }
    const float daylight = std::clamp((sunY + 0.08f) / 0.28f, 0.0f, 1.0f);
    const float horizon = 1.0f - std::clamp(std::abs(sunY) * 5.0f, 0.0f, 1.0f);
    ::net::minecraft::world::light::PhysicalLight sun;
    sun.shape = ::net::minecraft::world::light::LightShape::Directional;
    sun.directionX = sunX;
    sun.directionY = sunY;
    sun.directionZ = sunZ;
    sun.red = 1.0f;
    sun.green = 0.96f - horizon * 0.25f;
    sun.blue = 0.88f - horizon * 0.48f;
    sun.intensity = daylight;
    ctx.world->lightRegistry().upsert(::net::minecraft::world::light::UnifiedLightRegistry::sunKey(), sun);
  }
  if(skyEvent.cancelVanilla) {
    return;
  }
  SkyMeshes& meshes = skyMeshes();
  if(!meshes.built) {
    buildSkyDomes(meshes);
  }
  const Vec3d sky = ctx.world->getSkyColor(ctx.camera, tickDelta);
  float skyR = static_cast<float>(sky.x);
  float skyG = static_cast<float>(sky.y);
  float skyB = static_cast<float>(sky.z);
  const float starAlpha = 1.0f - ctx.world->getRainGradient(tickDelta);
  const float starBrightness = ctx.world->calculateSkyLightIntensity(tickDelta) * starAlpha;
  const gl::preset::SkyDomeDraw skyCaps;
  gl::color3f(skyR, skyG, skyB);
  Tessellator::drawMesh(meshes.lightSky);
  {
    const gl::preset::SkyDomeBackgroundFan backgroundCaps;
    gl::Lighting::turnOff();
    if(std::array<float, 4>* background = ctx.world->dimension->getBackgroundColor(timeOfDay, tickDelta);
       background != nullptr) {
      drawBackgroundFan(ctx, tickDelta, *background);
    }
  }
  {
    const gl::preset::SkyDomeCelestial celestialCaps;
    gl::MatrixGuard celestialMatrix;
    gl::rotatef(skyEvent.skyYawDegrees, 0.0f, 1.0f, 0.0f);
    gl::rotatef(skyEvent.celestialAngle * 180.0f / kPi, 1.0f, 0.0f, 0.0f);
    drawSunMoon(ctx, starAlpha);
  }
  {
    const gl::preset::SkyDomeStarsPass starsCaps;
    mod::WorldRenderEvent starsEvent = skyEvent;
    starsEvent.cancelVanilla = false;
    starsEvent.vanillaStageRan = false;
    starsEvent.starBrightness = starBrightness;
    starsEvent.rainStrength = ctx.world->getRainGradient(tickDelta);
    starsEvent.starsEnabled = ctx.options.stars;
    publishRenderStage(starsEvent, mod::WorldRenderStage::Stars, mod::RenderHookMoment::Before);
    if(starsEvent.starBrightness > 0.0f && ctx.options.stars && !starsEvent.cancelVanilla) {
      gl::color4f(starsEvent.starBrightness, starsEvent.starBrightness, starsEvent.starBrightness, starsEvent.starBrightness);
      Tessellator::drawMesh(meshes.stars);
      starsEvent.vanillaStageRan = true;
    }
    publishRenderStage(starsEvent, mod::WorldRenderStage::Stars, mod::RenderHookMoment::After);
  }
  gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
  gl::setCap(gl::cap::Blend, false);
  gl::setCap(gl::cap::AlphaTest, true);
  gl::setCap(gl::cap::Fog, true);
  gl::setCap(gl::cap::Texture2D, false);
  if(ctx.world->dimension->hasGround()) {
    gl::color3f(skyR * 0.2f + 0.04f, skyG * 0.2f + 0.04f, skyB * 0.6f + 0.1f);
  } else {
    gl::color3f(skyR, skyG, skyB);
  }
  Tessellator::drawMesh(meshes.darkSky);
  gl::setCap(gl::cap::Texture2D, true);
  gl::depthMask(true);
  skyEvent.vanillaStageRan = true;
  publishRenderStage(skyEvent, mod::WorldRenderStage::Sky, mod::RenderHookMoment::After);
}
} // namespace net::minecraft::client::render::atmosphere
