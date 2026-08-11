#include "net/minecraft/client/render/atmosphere/SkyDome.hpp"
#include <array>
#include <cmath>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereContext.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
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
 constexpr float r = 100.0f;
 tessellator.start(gl::prim::Quads);
 tessellator.vertex(-r, 16.0f, -r);
 tessellator.vertex(r, 16.0f, -r);
 tessellator.vertex(r, 16.0f, r);
 tessellator.vertex(-r, 16.0f, r);
 tessellator.vertex(-r, 0.0f, -r);
 tessellator.vertex(r, 0.0f, -r);
 tessellator.vertex(r, 16.0f, -r);
 tessellator.vertex(-r, 16.0f, -r);
 tessellator.vertex(-r, 16.0f, r);
 tessellator.vertex(r, 16.0f, r);
 tessellator.vertex(r, 0.0f, r);
 tessellator.vertex(-r, 0.0f, r);
 tessellator.vertex(-r, 0.0f, -r);
 tessellator.vertex(-r, 16.0f, -r);
 tessellator.vertex(-r, 16.0f, r);
 tessellator.vertex(-r, 0.0f, r);
 tessellator.vertex(r, 0.0f, -r);
 tessellator.vertex(r, 0.0f, r);
 tessellator.vertex(r, 16.0f, r);
 tessellator.vertex(r, 16.0f, -r);
 meshes.lightSky = tessellator.takeMesh();
 (void)meshes.lightSky.uploadToGpu();
 tessellator.start(gl::prim::Quads);
 tessellator.vertex(-r, -16.0f, r);
 tessellator.vertex(r, -16.0f, r);
 tessellator.vertex(r, -16.0f, -r);
 tessellator.vertex(-r, -16.0f, -r);
 tessellator.vertex(-r, -16.0f, -r);
 tessellator.vertex(r, -16.0f, -r);
 tessellator.vertex(r, 0.0f, -r);
 tessellator.vertex(-r, 0.0f, -r);
 tessellator.vertex(-r, 0.0f, r);
 tessellator.vertex(r, 0.0f, r);
 tessellator.vertex(r, -16.0f, r);
 tessellator.vertex(-r, -16.0f, r);
 tessellator.vertex(-r, -16.0f, -r);
 tessellator.vertex(-r, 0.0f, -r);
 tessellator.vertex(-r, 0.0f, r);
 tessellator.vertex(-r, -16.0f, r);
 tessellator.vertex(r, -16.0f, -r);
 tessellator.vertex(r, -16.0f, r);
 tessellator.vertex(r, 0.0f, r);
 tessellator.vertex(r, 0.0f, -r);
 meshes.darkSky = tessellator.takeMesh();
 (void)meshes.darkSky.uploadToGpu();
 tessellator.setCaptureOnly(false);
 meshes.built = true;
}
void drawBackgroundFan(const AtmosphereContext& ctx, float tickDelta, const std::array<float, 4>& bg) {
 const core::RenderStageScope stage(core::RenderStage::Sunset);
 const float timeOfDay = ctx.world->getTime(tickDelta);
 const core::ScopedDrawCameraState fanGuard;
 net::minecraft::util::math::Matrix4f fanPose = core::drawPose();
 fanPose.rotate(-90.0f, 0.0f, 1.0f, 0.0f);
 fanPose.rotate(90.0f, 1.0f, 0.0f, 0.0f);
 fanPose.rotate(timeOfDay > 0.5f ? 180.0f : 0.0f, 0.0f, 0.0f, 1.0f);
 core::setDrawPose(fanPose);
 Tessellator& tessellator = Tessellator::INSTANCE;
 tessellator.start(gl::prim::TriangleFan);
 tessellator.color(bg[0], bg[1], bg[2], bg[3]);
 tessellator.vertex(0.0, 100.0, 0.0);
 tessellator.color(bg[0], bg[1], bg[2], 0.0f);
 for(int i = 0; i <= 16; ++i) {
  const float angle = static_cast<float>(i) * kPi * 2.0f / 16.0f;
  const float sinA = std::sin(angle);
  const float cosA = std::cos(angle);
  tessellator.vertex(sinA * 120.0, cosA * 120.0, -cosA * 40.0f * bg[3]);
 }
 tessellator.draw();
}
void drawCelestialQuad(Tessellator& tessellator, const float direction[3], double halfSize, bool moon) {
 const double center[3] = {static_cast<double>(direction[0]) * 100.0,
                           static_cast<double>(direction[1]) * 100.0,
                           static_cast<double>(direction[2]) * 100.0};
 const double view[3] = {-direction[0], -direction[1], -direction[2]};
 const double reference[3] = {0.0, std::abs(view[1]) < 0.99 ? 1.0 : 0.0,
                              std::abs(view[1]) < 0.99 ? 0.0 : 1.0};
 double right[3] = {view[1] * reference[2] - view[2] * reference[1],
                    view[2] * reference[0] - view[0] * reference[2],
                    view[0] * reference[1] - view[1] * reference[0]};
 const double rightLength = std::sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
 for(double& value : right) {
  value = value / rightLength * halfSize;
 }
 const double up[3] = {right[1] * view[2] - right[2] * view[1],
                       right[2] * view[0] - right[0] * view[2],
                       right[0] * view[1] - right[1] * view[0]};
 const double u0 = moon ? 1.0 : 0.0;
 const double u1 = moon ? 0.0 : 1.0;
 tessellator.vertex(center[0] - right[0] + up[0], center[1] - right[1] + up[1],
                    center[2] - right[2] + up[2], u0, 0.0);
 tessellator.vertex(center[0] + right[0] + up[0], center[1] + right[1] + up[1],
                    center[2] + right[2] + up[2], u1, 0.0);
 tessellator.vertex(center[0] + right[0] - up[0], center[1] + right[1] - up[1],
                    center[2] + right[2] - up[2], u1, 1.0);
 tessellator.vertex(center[0] - right[0] - up[0], center[1] - right[1] - up[1],
                    center[2] - right[2] - up[2], u0, 1.0);
}
void drawSunMoon(const AtmosphereContext& ctx, float starAlpha, const render::CelestialState& celestial) {
 Tessellator& tessellator = Tessellator::INSTANCE;
 if(ctx.settings.renderSun) {
  const core::RenderStageScope stage(core::RenderStage::Sun);
  if(ctx.textureManager != nullptr) {
   core::activeTexture(gl::tex::Texture0);
   ctx.textureManager->bindTexture(ctx.textureManager->getTextureId("/terrain/sun.png"));
  }
  tessellator.startQuads();
  tessellator.color(1.0f, 1.0f, 1.0f, starAlpha);
  drawCelestialQuad(tessellator, celestial.sunDirectionWorld, 30.0, false);
  tessellator.draw();
 }
 if(ctx.settings.renderMoon) {
  const core::RenderStageScope stage(core::RenderStage::Moon);
  if(ctx.textureManager != nullptr) {
   core::activeTexture(gl::tex::Texture0);
   ctx.textureManager->bindTexture(ctx.textureManager->getTextureId("/terrain/moon.png"));
  }
  tessellator.startQuads();
  tessellator.color(1.0f, 1.0f, 1.0f, starAlpha);
  drawCelestialQuad(tessellator, celestial.moonDirectionWorld, 20.0, true);
  tessellator.draw();
 }
}
} // namespace
void renderSkyDome(const AtmosphereContext& ctx, float tickDelta) {
 if(ctx.world == nullptr || ctx.world->dimension == nullptr || ctx.camera == nullptr ||
    ctx.world->dimension->isNether) {
  return;
 }
 const float celestialAngle = render::core::celestialState().celestialAngle;
 const Vec3d sky = ctx.world->getSkyColor(ctx.camera, tickDelta);
 const float skyR = static_cast<float>(sky.x);
 const float skyG = static_cast<float>(sky.y);
 const float skyB = static_cast<float>(sky.z);
 const float starAlpha = 1.0f - ctx.world->getRainGradient(tickDelta);
 const float starBrightness = ctx.world->calculateSkyLightIntensity(tickDelta) * starAlpha;
 {
  render::core::SkyUniforms su = render::core::skyUniforms();
  su.renderStars = ctx.settings.renderStars;
  su.skyColor[0] = skyR;
  su.skyColor[1] = skyG;
  su.skyColor[2] = skyB;
  su.starBrightness = starBrightness;
  render::core::setSkyUniforms(su);
 }
 const RenderPassScope skyPass(RenderType::sky());
 core::depthMask(false);
 SkyMeshes& meshes = skyMeshes();
 if(!meshes.built) {
  buildSkyDomes(meshes);
 }
 if(ctx.settings.renderSky) {
  core::setConstColor(skyR, skyG, skyB, 1.0f);
  Tessellator::drawMesh(meshes.lightSky, core::constColorPacked());
  {
   core::enableBlend();
   core::blendAlpha();
   if(std::array<float, 4>* background = ctx.world->dimension->getBackgroundColor(celestialAngle, tickDelta);
      background != nullptr) {
    drawBackgroundFan(ctx, tickDelta, *background);
   }
  }
 }
 {
  const RenderPassScope sunMoonPass(RenderType::skyTextured());
  drawSunMoon(ctx, starAlpha, render::core::celestialState());
 }
 core::disableBlend();
 core::depthMask(true);
}
void renderSkyStars(const AtmosphereContext& ctx, float, float starBrightness) {
 if(ctx.world == nullptr || ctx.world->dimension == nullptr || ctx.camera == nullptr ||
    ctx.world->dimension->isNether || starBrightness <= 0.0f) {
  return;
 }
 const RenderPassScope skyPass(RenderType::sky());
 core::depthMask(false);
 SkyMeshes& meshes = skyMeshes();
 if(!meshes.built) {
  buildSkyDomes(meshes);
 }
 const core::RenderStageScope stage(core::RenderStage::Stars);
 core::setConstColor(starBrightness, starBrightness, starBrightness, starBrightness);
 Tessellator::drawMesh(meshes.stars, core::constColorPacked());
 core::depthMask(true);
}
void renderSkyVoid(const AtmosphereContext& ctx, float tickDelta) {
 if(ctx.world == nullptr || ctx.world->dimension == nullptr || ctx.camera == nullptr ||
    ctx.world->dimension->isNether || !ctx.settings.renderSky) {
  return;
 }
 const Vec3d sky = ctx.world->getSkyColor(ctx.camera, tickDelta);
 const float skyR = static_cast<float>(sky.x);
 const float skyG = static_cast<float>(sky.y);
 const float skyB = static_cast<float>(sky.z);
 const RenderPassScope skyPass(RenderType::sky());
 core::depthMask(false);
 SkyMeshes& meshes = skyMeshes();
 if(!meshes.built) {
  buildSkyDomes(meshes);
 }
 if(ctx.settings.renderSky) {
  const bool darkVoid = ctx.world->dimension->hasGround();
  const float voidR = darkVoid ? skyR * 0.2f + 0.04f : skyR;
  const float voidG = darkVoid ? skyG * 0.2f + 0.04f : skyG;
  const float voidB = darkVoid ? skyB * 0.6f + 0.1f : skyB;
  core::setConstColor(voidR, voidG, voidB, 1.0f);
  {
   const core::RenderStageScope stage(core::RenderStage::Void);
   Tessellator::drawMesh(meshes.darkSky, core::constColorPacked());
  }
 }
 core::depthMask(true);
}
} // namespace net::minecraft::client::render::atmosphere
