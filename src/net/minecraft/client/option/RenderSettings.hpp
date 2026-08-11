#pragma once
#include <string_view>
#include "net/minecraft/client/option/RenderDistance.hpp"
namespace net::minecraft {
class World;
struct Vec3d;
} // namespace net::minecraft
namespace net::minecraft::entity {
class Entity;
}
namespace net::minecraft::client::render {
struct PackDefinition;
}
namespace net::minecraft::client::option {
class GameOptions;
struct RenderSettings {
 float fovOffset = 0.0f;
 bool ambientOcclusionActive = false;
 float ambientOcclusionStrength = 0.0f;
 // Iris shaders.properties: separateAo / oldLighting (defaults match Iris).
 bool separateAo = false;
 bool oldLighting = false;
 bool mipmapLinearFilter = false;
 bool fancyLeaves = true;
 int leafInteriorFaces = 1;
 bool fancyGrass = true;
 bool renderWater = true;
 bool fancyWater = true;
 bool clearWater = false;
 float renderScale = 1.0f;
 // One value. `renderDistance.blocks` for camera/fog/uniforms,
 // `renderDistance.chunks()` for the section ring — never a stored copy of either.
 RenderDistance renderDistance{};
 int residentChunkRadius = 15;
 bool smoothInput = false;
 bool smoothFps = false;
 float chunkUpdatesSlider = 0.5f;
 bool chunkUpdatesDynamic = false;
 bool renderSky = true;
 bool renderStars = true;
 bool renderClouds = true;
 bool renderSun = true;
 bool renderMoon = true;
 // Iris shaders.properties underwaterOverlay / vignette (default on).
 bool underwaterOverlay = true;
 bool vignette = true;
 bool fancyClouds = true;
 float cloudHeightScale = 0.0f;
 bool fancyPrecipitation = true;
 float entityDistanceScale = 1.0f;
 bool weatherEnabled = true;
 // Iris rain.depth — weather writes depth when true (default false).
 bool rainDepth = false;
 // Iris frustum.culling / occlusion.culling pack overrides (default on).
 bool frustumCulling = true;
 bool occlusionCulling = true;
 // Iris backFace.* — when false, force cull off for that terrain layer.
 bool backFaceSolid = true;
 bool backFaceCutout = true;
 bool backFaceTranslucent = true;
 int rainMode = 0;
 bool animatedWater = true;
 bool animatedLava = true;
 bool animatedFire = true;
 bool animatedPortal = true;
 bool animatedRedstone = true;
 bool animatedExplosion = true;
 bool animatedFlame = true;
 bool animatedSmoke = true;
};
[[nodiscard]] RenderSettings renderSettings(const GameOptions& options);
void applyShaderPack(RenderSettings& settings, const render::PackDefinition& pack);
// `pack` is always a real definition - vanillaPackDefinition() when none is loaded - so
// the directives are applied unconditionally instead of being skipped for vanilla.
[[nodiscard]] RenderSettings renderSettings(const GameOptions& options,
                                            const render::PackDefinition& pack);
[[nodiscard]] float adjustFieldOfView(float baseFov, const RenderSettings& settings) noexcept;
[[nodiscard]] float cloudHeightOffset(float baseHeight, const RenderSettings& settings) noexcept;
[[nodiscard]] int chunkUpdatesPerPass(const RenderSettings& settings, int dirtyChunkCount = 0) noexcept;
[[nodiscard]] float rainGradient(const RenderSettings& settings, const World* world, float tickDelta);
[[nodiscard]] float thunderGradient(const RenderSettings& settings, const World* world, float tickDelta);
[[nodiscard]] bool shouldRenderEntity(const RenderSettings& settings,
                                      const net::minecraft::entity::Entity& entity,
                                      const Vec3d& cameraPos);
[[nodiscard]] bool shouldSpawnParticle(const RenderSettings& settings, std::string_view particle);
} // namespace net::minecraft::client::option
