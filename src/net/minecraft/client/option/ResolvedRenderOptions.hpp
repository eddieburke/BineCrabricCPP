#pragma once
#include <string>
#include <string_view>

#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::client::option {
struct ResolvedRenderOptions {
    float fovOffset = 0.0f;
    bool ambientOcclusionActive = false;
    float ambientOcclusionStrength = 0.0f;
    float brightnessBoost = 0.0f;
    bool mipmapLinearFilter = false;
    bool fancyLeaves = true;
    bool fancyGrass = true;
    bool renderWater = true;
    bool fancyWater = true;
    bool clearWater = false;
    bool customFog = false;
    bool customFogLinear = false;
    bool customFogColor = false;
    bool sphericalFog = true;
    float fogColorRed = 0.0f;
    float fogColorGreen = 0.0f;
    float fogColorBlue = 0.0f;
    float fogStart = 0.2f;
    float fogEnd = 0.8f;
    float fogDensity = 0.1f;
    int viewDistanceSetting = 0;
    float renderScale = 1.0f;
    float renderDistanceBlocks = 256.0f;
    float fogColorBlend = 0.0f;
    int chunkRadius = 12;
    int residentChunkRadius = 15;
    bool chunkVbo = false;
    bool smoothInput = false;
    bool smoothFps = false;
    float chunkUpdatesSlider = 0.5f;
    bool chunkUpdatesDynamic = false;
    bool renderSky = true;
    bool renderStars = true;
    bool renderClouds = true;
    bool fancyClouds = true;
    float cloudHeightScale = 0.0f;
    bool fancyPrecipitation = true;
    float entityDistanceScale = 1.0f;
    bool weatherEnabled = true;
    int rainMode = 0;
    bool animatedWater = true;
    bool animatedLava = true;
    bool animatedFire = true;
    bool animatedPortal = true;
    bool animatedRedstone = true;
    bool animatedExplosion = true;
    bool animatedFlame = true;
    bool animatedSmoke = true;
    bool fastDebugInfo = false;
};

[[nodiscard]] ResolvedRenderOptions resolve(const GameOptions& options);
[[nodiscard]] float adjustFieldOfView(float baseFov, const ResolvedRenderOptions& resolved) noexcept;
[[nodiscard]] float scaleAoCorner(float cornerBrightness, const ResolvedRenderOptions& resolved) noexcept;
[[nodiscard]] float applyBrightnessBoost(float luminance, const ResolvedRenderOptions& resolved) noexcept;
[[nodiscard]] float cloudHeightOffset(float baseHeight, const ResolvedRenderOptions& resolved) noexcept;
[[nodiscard]] int chunkUpdatesPerPass(const ResolvedRenderOptions& resolved,
                                      int dirtyChunkCount = 0,
                                      float gridAreaScale = 1.0f) noexcept;
[[nodiscard]] float rainGradient(const ResolvedRenderOptions& resolved, const World* world, float tickDelta);
[[nodiscard]] float thunderGradient(const ResolvedRenderOptions& resolved, const World* world, float tickDelta);
[[nodiscard]] bool shouldRenderEntity(const ResolvedRenderOptions& resolved,
                                      const Entity& entity,
                                      const Vec3d& cameraPos);
[[nodiscard]] bool shouldSpawnParticle(const ResolvedRenderOptions& resolved, std::string_view particle);
}  // namespace net::minecraft::client::option
