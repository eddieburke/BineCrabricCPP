#include "net/minecraft/client/option/RenderSettings.hpp"
#include <algorithm>
#include <cmath>
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPack.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::client::option {
RenderSettings renderSettings(const GameOptions& options) {
 RenderSettings r{};
 r.fovOffset = options.fieldOfView * 40.0f;
 r.ambientOcclusionActive = options.ao;
 r.ambientOcclusionStrength = options.ao ? std::clamp(options.aoLevel, 0.0f, 1.0f) : 0.0f;
 r.brightnessBoost = options.brightness;
 r.mipmapLinearFilter = options.mipmapLinear;
 r.fancyLeaves = options.trees == 0;
 r.fancyGrass = (options.grass & 1) == 0;
 r.renderWater = options.water < 2;
 r.fancyWater = options.water == 0 && options.fancyGraphics;
 r.clearWater = options.clearWater;
 r.viewDistanceSetting = options.viewDistance & 3;
 r.renderScale = std::isfinite(options.renderScale) ? std::clamp(options.renderScale, 1.0f, 5.0f) : 1.0f;
 const int baseDistance = 256 >> r.viewDistanceSetting;
 r.renderDistanceBlocks = static_cast<float>(baseDistance) * r.renderScale;
 const float distanceBlend = 1.0f / static_cast<float>(4 - r.viewDistanceSetting);
 r.fogColorBlend = 1.0f - static_cast<float>(std::pow(static_cast<double>(distanceBlend), 0.25));
 const int visualGridDiameter =
     std::clamp(static_cast<int>(static_cast<float>(baseDistance * 2) * r.renderScale), 64, 2560);
 r.chunkRadius = (visualGridDiameter / 16 + 1) / 2;
 const int preloadMargin = options.preloadedChunks <= 0 ? 3 : 3 + options.preloadedChunks / 2;
 r.residentChunkRadius = r.chunkRadius + preloadMargin;
 r.smoothInput = options.smoothInput;
 r.smoothFps = options.smoothFps;
 r.chunkUpdatesSlider = std::isfinite(options.chunkUpdates) ? std::clamp(options.chunkUpdates, 0.0f, 1.0f) : 0.5f;
 r.chunkUpdatesDynamic = options.chunkUpdatesDynamic;
 r.renderSky = options.sky;
 r.renderStars = options.stars;
 r.vignette = options.vignette;
 r.underwaterOverlay = options.underwaterOverlay;
 r.renderClouds = (options.clouds & 3) != 2;
 r.fancyClouds = (options.clouds & 3) == 0 && options.fancyGraphics;
 r.cloudHeightScale = options.cloudsHeight;
 r.fancyPrecipitation = options.rain == 0 && options.fancyGraphics;
 r.entityDistanceScale = std::clamp(options.entityDistanceScale, 0.25f, 4.0f);
 r.weatherEnabled = options.weather;
 r.rainMode = options.rain;
 r.frustumCulling = options.frustumCulling;
 r.occlusionCulling = true;
 r.animatedWater = options.animatedWater == 0;
 r.animatedLava = options.animatedLava == 0;
 r.animatedFire = options.animatedFire;
 r.animatedPortal = options.animatedPortal;
 r.animatedRedstone = options.animatedRedstone;
 r.animatedExplosion = options.animatedExplosion;
 r.animatedFlame = options.animatedFlame;
 r.animatedSmoke = options.animatedSmoke;
 return r;
}
void applyShaderPack(RenderSettings& settings, const render::shaderpack::ShaderPackDefinition& pack) {
 settings.ambientOcclusionStrength *= std::clamp(pack.ambientOcclusionLevel, 0.0f, 1.0f);
 settings.separateAo = pack.separateAo;
 settings.oldLighting = pack.oldLighting;
 if(!pack.renderClouds) {
  settings.renderClouds = false;
 } else if(pack.cloudsMode == "fast") {
  settings.renderClouds = true;
  settings.fancyClouds = false;
 } else if(pack.cloudsMode == "fancy") {
  settings.renderClouds = true;
  settings.fancyClouds = true;
 }
 if(!pack.renderSky) {
  settings.renderSky = false;
 }
 if(!pack.renderStars) {
  settings.renderStars = false;
 }
 if(!pack.renderWeather) {
  settings.weatherEnabled = false;
 }
 settings.renderSun = pack.renderSun;
 settings.renderMoon = pack.renderMoon;
 settings.underwaterOverlay = settings.underwaterOverlay && pack.underwaterOverlay;
 settings.vignette = settings.vignette && pack.vignette;
 settings.rainDepth = pack.rainDepth;
 settings.frustumCulling = settings.frustumCulling && pack.frustumCulling;
 settings.occlusionCulling = pack.occlusionCulling;
 settings.backFaceSolid = pack.backFaceSolid;
 settings.backFaceCutout = pack.backFaceCutout;
 settings.backFaceTranslucent = pack.backFaceTranslucent;
}
RenderSettings renderSettings(const GameOptions& options, const render::shaderpack::ShaderPackDefinition* pack) {
 RenderSettings settings = renderSettings(options);
 if(pack != nullptr) {
  applyShaderPack(settings, *pack);
 }
 return settings;
}
float adjustFieldOfView(float baseFov, const RenderSettings& resolved) noexcept {
 return baseFov + resolved.fovOffset;
}
float scaleAoCorner(float cornerBrightness, const RenderSettings& resolved) noexcept {
 // Soft AO strength only. Brightness is applied once: in lightmap for flat
 // lighting, or via applyBrightnessBoost on baked AO corners (fullbright lm).
 return 1.0f - (1.0f - cornerBrightness) * resolved.ambientOcclusionStrength;
}
float applyBrightnessBoost(float luminance, const RenderSettings& resolved) noexcept {
 const float boost = resolved.brightnessBoost;
 if(boost <= 0.0f) return luminance;
 const float exponent = 1.0f / (1.0f + boost * 4.0f);
 return std::pow(luminance, exponent);
}
float cloudHeightOffset(float baseHeight, const RenderSettings& resolved) noexcept {
 return baseHeight + resolved.cloudHeightScale * 128.0f;
}
int chunkUpdatesPerPass(const RenderSettings& resolved, int dirtyChunkCount) noexcept {
 constexpr int kMinBudget = 1;
 constexpr int kMaxBudget = 16;
 const float slider =
     std::isfinite(resolved.chunkUpdatesSlider) ? std::clamp(resolved.chunkUpdatesSlider, 0.0f, 1.0f) : 0.5f;
 int budget = kMinBudget + static_cast<int>(std::lround(slider * static_cast<float>(kMaxBudget - kMinBudget)));
 budget = std::clamp(budget, kMinBudget, kMaxBudget);
 if(resolved.chunkUpdatesDynamic) {
  const int backlog = std::max(0, dirtyChunkCount);
  const int excess = std::max(0, backlog - budget * 2);
  budget = std::min(kMaxBudget, budget + (excess + 7) / 8);
 }
 return budget;
}
float rainGradient(const RenderSettings& resolved, const World* world, float tickDelta) {
 if(!resolved.weatherEnabled || resolved.rainMode >= 2) {
  return 0.0f;
 }
 return world != nullptr ? world->getRainGradient(tickDelta) : 0.0f;
}
float thunderGradient(const RenderSettings& resolved, const World* world, float tickDelta) {
 if(!resolved.weatherEnabled) {
  return 0.0f;
 }
 return world != nullptr ? world->getThunderGradient(tickDelta) : 0.0f;
}
bool shouldRenderEntity(const RenderSettings& resolved,
                        const net::minecraft::entity::Entity& entity,
                        const Vec3d& cameraPos) {
 const double dx = entity.x - cameraPos.x;
 const double dy = entity.y - cameraPos.y;
 const double dz = entity.z - cameraPos.z;
 const double distSq = dx * dx + dy * dy + dz * dz;
 const double sideLength = std::max({entity.boundingBox.maxX - entity.boundingBox.minX,
                                     entity.boundingBox.maxY - entity.boundingBox.minY,
                                     entity.boundingBox.maxZ - entity.boundingBox.minZ});
 const double scaled = sideLength * 64.0 * entity.renderDistanceMultiplier * resolved.entityDistanceScale;
 return distSq < scaled * scaled;
}
bool shouldSpawnParticle(const RenderSettings& resolved, std::string_view particle) {
 if(particle == "bubble" || particle == "splash") {
  return resolved.animatedWater;
 }
 if(particle == "lava") {
  return resolved.animatedLava;
 }
 if(particle == "flame") {
  return resolved.animatedFlame;
 }
 if(particle == "fire") {
  return resolved.animatedFire;
 }
 if(particle == "smoke" || particle == "largesmoke") {
  return resolved.animatedSmoke;
 }
 if(particle == "portal") {
  return resolved.animatedPortal;
 }
 if(particle == "reddust") {
  return resolved.animatedRedstone;
 }
 if(particle == "explode") {
  return resolved.animatedExplosion;
 }
 return true;
}
} // namespace net::minecraft::client::option
