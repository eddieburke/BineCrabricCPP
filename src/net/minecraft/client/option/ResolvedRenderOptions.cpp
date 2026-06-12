#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"

#include <algorithm>
#include <cmath>

namespace net::minecraft::client::option {
namespace {

int baseViewDistanceBlocks(int viewDistanceSetting) noexcept
{
    return 256 >> (viewDistanceSetting & 3);
}

float renderDistanceScale(const GameOptions& options) noexcept
{
    return std::clamp(options.ofRenderScale, 1.0f, 5.0f);
}

} // namespace

ResolvedRenderOptions resolve(const GameOptions& options)
{
    ResolvedRenderOptions r {};
    const float scale = renderDistanceScale(options);
    const int baseViewDistance = baseViewDistanceBlocks(options.viewDistance);
    r.viewDistanceBlocks = static_cast<float>(baseViewDistance) * scale;

    // Java caps the renderer grid diameter at 400 blocks even when Far's fog
    // distance is 256. Keeping that cap avoids compiling a much larger torus
    // than the camera can use.
    int diameter = std::min(baseViewDistance * 2, 400);
    diameter = static_cast<int>(static_cast<float>(diameter) * scale);
    r.chunkGridRadius = std::clamp(diameter, 64, 2000);

    r.chunkPreloadRadius = options.ofPreloadedChunks <= 0 ? 2 : 2 + options.ofPreloadedChunks / 2;
    r.fovOffset = options.fieldOfView * 40.0f;

    r.ambientOcclusionActive = options.ao && options.ofAoLevel > 0.0f;
    r.ambientOcclusionStrength = r.ambientOcclusionActive
        ? std::clamp(options.ofAoLevel, 0.0f, 1.0f) : 0.0f;
    r.brightnessBoost = options.ofBrightness;

    r.mipmapLinearFilter = options.ofMipmapLinear;
    r.fancyLeaves = options.ofTrees == 0;
    r.fancyGrass = (options.ofGrass & 1) == 0;
    r.renderWater = options.ofWater < 2;
    r.fancyWater = options.ofWater == 0 && options.fancyGraphics;
    r.clearWater = options.ofClearWater;

    r.customFog = options.ofFogFancy;
    r.customFogColor = options.ofFogFancy && options.ofFogColorMode == 1;
    r.customFogLinear = options.ofFogFancy && options.ofFogMode == 0;
    r.sphericalFog = options.ofFogProjection == 0;
    r.fogColorRed = options.ofFogColorRed;
    r.fogColorGreen = options.ofFogColorGreen;
    r.fogColorBlue = options.ofFogColorBlue;
    r.fogStart = options.ofFogStart;
    r.fogEnd = options.ofFogEnd;
    r.fogDensity = options.ofFogDensity;

    r.chunkVbo = options.ofVBO || options.advancedOpengl >= 2;
    r.smoothInput = options.ofSmoothInput;
    r.smoothFps = options.ofSmoothFps;
    r.chunkUpdatesSlider = options.ofChunkUpdates;
    r.chunkUpdatesDynamic = options.ofChunkUpdatesDynamic;

    r.renderSky = options.ofSky;
    r.renderStars = options.ofStars;
    r.renderClouds = (options.ofClouds & 3) < 2;
    r.fancyClouds = (options.ofClouds & 3) == 0 && options.fancyGraphics;
    r.cloudHeightScale = options.ofCloudsHeight;

    r.fancyPrecipitation = options.ofRain == 0 && options.fancyGraphics;
    r.entityDistanceScale = options.ofEntityDistanceScale;

    r.weatherEnabled = options.ofWeather;
    r.rainMode = options.ofRain;

    r.animatedWater = options.ofAnimatedWater == 0;
    r.animatedLava = options.ofAnimatedLava == 0;
    r.animatedFire = options.ofAnimatedFire;
    r.animatedPortal = options.ofAnimatedPortal;
    r.animatedRedstone = options.ofAnimatedRedstone;
    r.animatedExplosion = options.ofAnimatedExplosion;
    r.animatedFlame = options.ofAnimatedFlame;
    r.animatedSmoke = options.ofAnimatedSmoke;

    r.fastDebugInfo = options.ofFastDebugInfo;
    return r;
}

float adjustFieldOfView(float baseFov, const ResolvedRenderOptions& resolved) noexcept
{
    return baseFov + resolved.fovOffset;
}

float scaleAoCorner(float cornerBrightness, const ResolvedRenderOptions& resolved) noexcept
{
    return 1.0f - (1.0f - cornerBrightness) * resolved.ambientOcclusionStrength;
}

float applyBrightnessBoost(float luminance, const ResolvedRenderOptions& resolved) noexcept
{
    return luminance + resolved.brightnessBoost * (1.0f - luminance);
}

float cloudHeightOffset(float baseHeight, const ResolvedRenderOptions& resolved) noexcept
{
    return baseHeight + resolved.cloudHeightScale * 128.0f;
}

int chunkUpdatesPerPass(const ResolvedRenderOptions& resolved, int dirtyChunkCount) noexcept
{
    int budget = 1 + static_cast<int>(std::lround(resolved.chunkUpdatesSlider * 4.0f));
    budget = std::clamp(budget, 1, 5);
    if (resolved.chunkUpdatesDynamic && dirtyChunkCount > budget * 4) {
        budget = 5;
    }
    return budget;
}

float rainGradient(const ResolvedRenderOptions& resolved, const World* world, float tickDelta)
{
    if (!resolved.weatherEnabled || resolved.rainMode >= 2) {
        return 0.0f;
    }
    return world != nullptr ? world->getRainGradient(tickDelta) : 0.0f;
}

float thunderGradient(const ResolvedRenderOptions& resolved, const World* world, float tickDelta)
{
    if (!resolved.weatherEnabled) {
        return 0.0f;
    }
    return world != nullptr ? world->getThunderGradient(tickDelta) : 0.0f;
}

bool shouldRenderEntity(const ResolvedRenderOptions& resolved, const Entity& entity, const Vec3d& cameraPos)
{
    const double dx = entity.x - cameraPos.x;
    const double dy = entity.y - cameraPos.y;
    const double dz = entity.z - cameraPos.z;
    const double distSq = dx * dx + dy * dy + dz * dz;
    const double sideLength = std::max({
        entity.boundingBox.maxX - entity.boundingBox.minX,
        entity.boundingBox.maxY - entity.boundingBox.minY,
        entity.boundingBox.maxZ - entity.boundingBox.minZ});
    const double scaled = sideLength * 64.0 * entity.renderDistanceMultiplier * resolved.entityDistanceScale;
    return distSq < scaled * scaled;
}

bool shouldSpawnParticle(const ResolvedRenderOptions& resolved, std::string_view particle)
{
    if (particle == "bubble" || particle == "splash") {
        return resolved.animatedWater;
    }
    if (particle == "lava") {
        return resolved.animatedLava;
    }
    if (particle == "flame") {
        return resolved.animatedFlame;
    }
    if (particle == "fire") {
        return resolved.animatedFire;
    }
    if (particle == "smoke" || particle == "largesmoke") {
        return resolved.animatedSmoke;
    }
    if (particle == "portal") {
        return resolved.animatedPortal;
    }
    if (particle == "reddust") {
        return resolved.animatedRedstone;
    }
    if (particle == "explode") {
        return resolved.animatedExplosion;
    }
    return true;
}

} // namespace net::minecraft::client::option
