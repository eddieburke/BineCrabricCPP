#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include <algorithm>
#include <cmath>
namespace net::minecraft::client::option {
ResolvedRenderOptions resolve(const GameOptions& options) {
  ResolvedRenderOptions r{};
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
  r.customFog = options.fogFancy;
  r.customFogColor = options.fogFancy && options.fogColorMode == 1;
  r.customFogLinear = options.fogFancy && options.fogMode == 0;
  r.sphericalFog = options.fogProjection == 0;
  r.fogColorRed = options.fogColorRed;
  r.fogColorGreen = options.fogColorGreen;
  r.fogColorBlue = options.fogColorBlue;
  r.fogStart = options.fogStart;
  r.fogEnd = options.fogEnd;
  r.fogDensity = options.fogDensity;
  r.viewDistanceSetting = options.viewDistance & 3;
  r.renderScale = std::isfinite(options.renderScale) ? std::clamp(options.renderScale, 1.0f, 5.0f) : 1.0f;
  const int baseDistance = 256 >> r.viewDistanceSetting;
  r.renderDistanceBlocks = static_cast<float>(baseDistance) * r.renderScale;
  const float distanceBlend = 1.0f / static_cast<float>(4 - r.viewDistanceSetting);
  r.fogColorBlend = 1.0f - static_cast<float>(std::pow(static_cast<double>(distanceBlend), 0.25));
  const int visualGridDiameter =
      std::clamp(static_cast<int>(static_cast<float>(std::min(baseDistance * 2, 400)) * r.renderScale), 64, 2000);
  r.chunkRadius = (visualGridDiameter / 16 + 1) / 2;
  const int preloadMargin = options.preloadedChunks <= 0 ? 3 : 3 + options.preloadedChunks / 2;
  r.residentChunkRadius = r.chunkRadius + preloadMargin;
  r.chunkVbo = options.vbo;
  r.smoothInput = options.smoothInput;
  r.smoothFps = options.smoothFps;
  r.chunkUpdatesSlider = options.chunkUpdates;
  r.chunkUpdatesDynamic = options.chunkUpdatesDynamic;
  r.renderSky = options.sky;
  r.renderStars = options.stars;
  r.renderClouds = (options.clouds & 3) != 2;
  r.fancyClouds = (options.clouds & 3) == 0 && options.fancyGraphics;
  r.cloudHeightScale = options.cloudsHeight;
  r.fancyPrecipitation = options.rain == 0 && options.fancyGraphics;
  r.entityDistanceScale = std::clamp(options.entityDistanceScale, 0.25f, 4.0f);
  r.weatherEnabled = options.weather;
  r.rainMode = options.rain;
  r.animatedWater = options.animatedWater == 0;
  r.animatedLava = options.animatedLava == 0;
  r.animatedFire = options.animatedFire;
  r.animatedPortal = options.animatedPortal;
  r.animatedRedstone = options.animatedRedstone;
  r.animatedExplosion = options.animatedExplosion;
  r.animatedFlame = options.animatedFlame;
  r.animatedSmoke = options.animatedSmoke;
  r.fastDebugInfo = options.fastDebugInfo;
  return r;
}
float adjustFieldOfView(float baseFov, const ResolvedRenderOptions& resolved) noexcept {
  return baseFov + resolved.fovOffset;
}
float scaleAoCorner(float cornerBrightness, const ResolvedRenderOptions& resolved) noexcept {
  return 1.0f - (1.0f - cornerBrightness) * resolved.ambientOcclusionStrength;
}
float applyBrightnessBoost(float luminance, const ResolvedRenderOptions& resolved) noexcept {
  return luminance + resolved.brightnessBoost * (1.0f - luminance);
}
float cloudHeightOffset(float baseHeight, const ResolvedRenderOptions& resolved) noexcept {
  return baseHeight + resolved.cloudHeightScale * 128.0f;
}
int chunkUpdatesPerPass(const ResolvedRenderOptions& resolved, int dirtyChunkCount, float gridAreaScale) noexcept {
  const float scale = std::clamp(gridAreaScale, 1.0f, 50.0f);
  int budget = 1 + static_cast<int>(std::lround(resolved.chunkUpdatesSlider * 4.0f * scale));
  budget = std::clamp(budget, 1, static_cast<int>(std::ceil(5.0f * scale)));
  if(resolved.chunkUpdatesDynamic && dirtyChunkCount > budget * 4) {
    budget = std::max(budget, static_cast<int>(std::ceil(5.0f * scale)));
  }
  return budget;
}
float rainGradient(const ResolvedRenderOptions& resolved, const World* world, float tickDelta) {
  if(!resolved.weatherEnabled || resolved.rainMode >= 2) {
    return 0.0f;
  }
  return world != nullptr ? world->getRainGradient(tickDelta) : 0.0f;
}
float thunderGradient(const ResolvedRenderOptions& resolved, const World* world, float tickDelta) {
  if(!resolved.weatherEnabled) {
    return 0.0f;
  }
  return world != nullptr ? world->getThunderGradient(tickDelta) : 0.0f;
}
bool shouldRenderEntity(const ResolvedRenderOptions& resolved, const Entity& entity, const Vec3d& cameraPos) {
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
bool shouldSpawnParticle(const ResolvedRenderOptions& resolved, std::string_view particle) {
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
