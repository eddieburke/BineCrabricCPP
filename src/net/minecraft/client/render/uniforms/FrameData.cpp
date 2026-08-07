#include "net/minecraft/client/render/uniforms/FrameData.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <limits>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/pipeline/Manager.hpp"
#include "net/minecraft/client/render/shaderpack/BiomeTables.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/entity/LightningEntity.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/vehicle/BoatEntity.hpp"
#include "net/minecraft/util/hit/HitResult.hpp"
#include "net/minecraft/util/hit/HitResultType.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/light/LightType.hpp"
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
namespace net::minecraft::client::render {
namespace {
struct FrameState {
 PackUniformValues previous{};
 PackUniformValues current{};
 bool initialized = false;
};
FrameState g_frameState;
CameraPositionTracker g_cameraTracker;
const auto g_shaderClockStart = std::chrono::steady_clock::now();
std::chrono::steady_clock::time_point g_previousFrameTime = g_shaderClockStart;
int g_frameCounter = 0;
float g_frameTimeCounterAccumulator = 0.0f;
float g_centerDepthSmooth = 0.0f;
float g_wetnessSmooth = 0.0f;
constexpr float kPiF = 3.14159265358979323846f;
void tickFrameClock(PackUniformValues& values) {
 const auto now = std::chrono::steady_clock::now();
 if(g_frameState.initialized) {
  g_frameState.previous = g_frameState.current;
  const float elapsed = std::chrono::duration<float>(now - g_previousFrameTime).count();
  values.frameTime = std::max(0.0f, std::floor(elapsed * 1000.0f) / 1000.0f);
 } else {
  values.frameTime = 0.0f;
 }
 values.frameCounter = g_frameCounter;
 g_frameCounter = (g_frameCounter + 1) % 720720;
 g_previousFrameTime = now;
 g_frameTimeCounterAccumulator += values.frameTime;
 if(g_frameTimeCounterAccumulator >= 3600.0f) {
  g_frameTimeCounterAccumulator = 0.0f;
 }
 values.frameTimeCounter = g_frameTimeCounterAccumulator;
}
void fillViewport(PackUniformValues& values, int width, int height) {
 values.viewWidth = static_cast<float>(width);
 values.viewHeight = static_cast<float>(height);
 values.aspectRatio = values.viewWidth / std::max(values.viewHeight, 1.0f);
}
void fillCameraPlanes(PackUniformValues& values, const FrameRenderCamera& camera) {
 // The pack-facing near/far pair is the main camera's planes.
 values.nearPlane = camera.nearPlane;
 values.farPlane = camera.farPlane;
}
void fillFog(PackUniformValues& values) {
 const auto& fog = render::core::fog();
 values.fogColor[0] = fog.color[0];
 values.fogColor[1] = fog.color[1];
 values.fogColor[2] = fog.color[2];
 values.fogDensity = fog.density;
 values.fogStart = fog.start;
 values.fogEnd = fog.end;
 values.fogMode = fog.enabled ? render::core::fogModeToGlConstant(fog.mode) : 0;
 values.fogShape = fog.enabled ? 1 : -1;
 if(fog.enabled) {
  values.irisFogColor[0] = fog.color[0];
  values.irisFogColor[1] = fog.color[1];
  values.irisFogColor[2] = fog.color[2];
  values.irisFogColor[3] = fog.color[3];
 } else {
  values.irisFogColor[0] = 1.0f;
  values.irisFogColor[1] = 1.0f;
  values.irisFogColor[2] = 1.0f;
  values.irisFogColor[3] = 1.0f;
 }
 values.irisFogStart = fog.start;
 values.irisFogEnd = fog.end;
 values.irisFogDensity = std::max(0.0f, fog.density);
 values.irisCurrentAlphaTest = render::core::alphaTestRef();
}
void fillFlags(PackUniformValues& values, int shadowMapResolution, bool normalAvailable, bool shadowAvailable) {
 values.normalAvailable = normalAvailable ? 1 : 0;
 values.shadowAvailable = shadowAvailable ? 1 : 0;
 values.shadowMapResolution = static_cast<float>(shadowMapResolution);
}
void fillSystemTime(PackUniformValues& values) {
 const std::time_t now = std::time(nullptr);
 std::tm local{};
#ifdef _WIN32
 localtime_s(&local, &now);
#else
 localtime_r(&now, &local);
#endif
 values.currentDate[0] = local.tm_year + 1900;
 values.currentDate[1] = local.tm_mon + 1;
 values.currentDate[2] = local.tm_mday;
 values.currentTime[0] = local.tm_hour;
 values.currentTime[1] = local.tm_min;
 values.currentTime[2] = local.tm_sec;
 values.currentYearTime[0] = local.tm_yday * 86400 + local.tm_hour * 3600 + local.tm_min * 60 + local.tm_sec;
 const int year = local.tm_year + 1900;
 const int days = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 366 : 365;
 values.currentYearTime[1] = days * 86400 - values.currentYearTime[0];
}
void splitCameraPosition(double x, double y, double z, int* outInt, float* outFract) {
 const auto split = [](double value, int& i, float& f) {
  const double floored = std::floor(value);
  i = static_cast<int>(floored);
  f = static_cast<float>(value - floored);
 };
 split(x, outInt[0], outFract[0]);
 split(y, outInt[1], outFract[1]);
 split(z, outInt[2], outFract[2]);
}
void fillCameraState(PackUniformValues& values,
                     const FrameRenderCamera& camera,
                     const FrameRenderCamera& shadowCamera,
                     bool shadowAvailable) {
 g_cameraTracker.update(camera.eyeX, camera.eyeY, camera.eyeZ);
 for(int axis = 0; axis < 3; ++axis) {
  values.cameraPosition[axis] = static_cast<float>(g_cameraTracker.current(axis));
 }
 splitCameraPosition(g_cameraTracker.currentUnshifted(0), g_cameraTracker.currentUnshifted(1),
                     g_cameraTracker.currentUnshifted(2), values.cameraPositionInt,
                     values.cameraPositionFract);
 for(int axis = 0; axis < 3; ++axis) {
  values.previousCameraPosition[axis] = static_cast<float>(g_cameraTracker.previous(axis));
 }
 splitCameraPosition(g_cameraTracker.previousUnshifted(0), g_cameraTracker.previousUnshifted(1),
                     g_cameraTracker.previousUnshifted(2), values.previousCameraPositionInt,
                     values.previousCameraPositionFract);
 render::buildCameraProjection(values.gbufferProjection, camera);
 render::buildCameraProjectionInverse(values.gbufferProjectionInverse, camera);
 buildCameraModelView(values.gbufferModelView, camera);
 buildCameraModelViewInverse(values.gbufferModelViewInverse, camera);
 std::copy(std::begin(g_frameState.previous.gbufferProjection), std::end(g_frameState.previous.gbufferProjection),
           values.gbufferPreviousProjection);
 std::copy(std::begin(g_frameState.previous.gbufferModelView), std::end(g_frameState.previous.gbufferModelView),
           values.gbufferPreviousModelView);
 render::directionToView(0.0f, 1.0f, 0.0f, camera, values.upPosition);
 if(shadowAvailable) {
  buildCameraModelView(values.shadowModelView, shadowCamera);
  render::buildCameraProjection(values.shadowProjection, shadowCamera);
  render::buildCameraProjectionInverse(values.shadowProjectionInverse, shadowCamera);
  buildCameraModelViewInverse(values.shadowModelViewInverse, shadowCamera);
 }
}
void fillBiome(PackUniformValues& values, const net::minecraft::World& world, const FrameRenderCamera& camera) {
 const Biome& biome = world.getBiome(static_cast<int>(camera.eyeX), static_cast<int>(camera.eyeZ));
 values.biome = static_cast<int>(biome.id);
 values.biomePrecipitation = biome.canSnow() ? 2 : biome.canRain() ? 1
                                                                   : 0;
 const BiomeClimate& climate = kBiomeClimates[static_cast<std::size_t>(biome.id)];
 values.biomeCategory = static_cast<int>(climate.category);
 values.temperature = climate.temperature;
 values.rainfall = climate.rainfall;
}
void fillWorldState(PackUniformValues& values,
                    const net::minecraft::World& world,
                    const FrameRenderCamera& camera,
                    float tickDelta) {
 const render::CelestialState& celestial = render::core::celestialState();
 const float celestialAngle = celestial.celestialAngle;
 values.sunAngle = celestial.sunAngle;
 values.shadowAngle = celestial.shadowAngle;
 const float worldSunX = celestial.sunDirectionWorld[0];
 const float worldSunY = celestial.sunDirectionWorld[1];
 const float worldSunZ = celestial.sunDirectionWorld[2];
 render::directionToView(worldSunX, worldSunY, worldSunZ, camera, values.sunPosition);
 render::directionToView(-worldSunX, -worldSunY, -worldSunZ, camera, values.moonPosition);
 const auto scaleTo100 = [](float v[3]) {
  const float len = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  if(len > 1e-6f) {
   const float s = 100.0f / len;
   v[0] *= s;
   v[1] *= s;
   v[2] *= s;
  }
 };
 scaleTo100(values.sunPosition);
 scaleTo100(values.moonPosition);
 const auto& sun = world.lightRegistry().sun();
 values.sunColor[0] = sun.red;
 values.sunColor[1] = sun.green;
 values.sunColor[2] = sun.blue;
 values.sunIntensity = sun.intensity;
 const std::uint64_t absoluteTime = world.getTime();
 if(world.clientTimeMode() == 1) {
  values.worldDay = static_cast<int>(absoluteTime / 24000ULL);
  values.worldTime = 6000;
 } else if(world.clientTimeMode() == 2) {
  values.worldDay = static_cast<int>(absoluteTime / 24000ULL);
  values.worldTime = 18000;
 } else {
  values.worldTime = static_cast<int>(absoluteTime % 24000ULL);
  values.worldDay = static_cast<int>(absoluteTime / 24000ULL);
 }
 values.moonPhase = static_cast<int>((absoluteTime / 24000ULL) % 8ULL);
 const net::minecraft::client::Minecraft* celestialClient = net::minecraft::client::Minecraft::INSTANCE;
 const PackDefinition& celestialDefinition =
     celestialClient != nullptr && celestialClient->gameRenderer != nullptr
         ? celestialClient->gameRenderer->packDefinition()
         : vanillaPackDefinition();
 const bool isEnd = world.dimension != nullptr && world.dimension->id == 1;
 if(isEnd && celestialDefinition.endFlashShadows) {
  std::copy(std::begin(values.endFlashPosition), std::end(values.endFlashPosition), values.shadowLightPosition);
 } else if(celestial.day) {
  std::copy(std::begin(values.sunPosition), std::end(values.sunPosition), values.shadowLightPosition);
 } else {
  std::copy(std::begin(values.moonPosition), std::end(values.moonPosition), values.shadowLightPosition);
 }
 values.rainStrength = world.getRainGradient(tickDelta);
 values.wetness = g_wetnessSmooth;
 values.thunderStrength = world.getThunderGradient(tickDelta);
 {
  const Vec3d sky = world.getSkyColor(
      net::minecraft::client::Minecraft::INSTANCE != nullptr ? net::minecraft::client::Minecraft::INSTANCE->camera
                                                             : nullptr,
      tickDelta);
  values.skyColor[0] = static_cast<float>(sky.x);
  values.skyColor[1] = static_cast<float>(sky.y);
  values.skyColor[2] = static_cast<float>(sky.z);
 }
 if(world.dimension != nullptr) {
  values.hasCeiling = world.dimension->hasCeiling ? 1 : 0;
  values.hasSkylight = world.dimension->hasCeiling ? 0 : 1;
  values.ambientLight = world.dimension->isNether ? 0.1f : 0.0f;
  values.bedrockLevel = 0;
  values.heightLimit = 128;
  values.logicalHeightLimit = 128;
  values.cloudHeight = world.dimension->hasCeiling
                           ? std::numeric_limits<float>::quiet_NaN()
                           : world.dimension->getCloudHeight();
  values.seaLevel = world.dimension->isNether ? 32 : isEnd ? 0
                                                           : 63;
 }
 values.cloudTime = static_cast<float>(static_cast<double>(absoluteTime % 102400ULL) + tickDelta) * 0.03f;
 fillBiome(values, world, camera);
 for(net::minecraft::Entity* entity : world.globalEntities) {
  if(dynamic_cast<LightningEntity*>(entity) == nullptr) continue;
  values.lightningBoltPosition[0] = static_cast<float>(entity->x - camera.eyeX);
  values.lightningBoltPosition[1] = static_cast<float>(entity->y - camera.eyeY);
  values.lightningBoltPosition[2] = static_cast<float>(entity->z - camera.eyeZ);
  values.lightningBoltPosition[3] = 1.0f;
  break;
 }
 values.entityId = render::core::entityId();
 {
  for(int c = 0; c < 3; ++c) {
   for(int r = 0; r < 3; ++r) {
    values.irisDefaultNormalMat[c * 3 + r] = values.gbufferModelViewInverse[r * 4 + c];
   }
  }
 }
}
struct SmoothedState {
 float up = 0.0f;
 float down = 0.0f;
 float value = 0.0f;
 bool initialized = false;
 void update(float target, float frameTime) {
  smoothExponential(target, value, initialized, frameTime, (target > value ? up : down) * 0.1f);
 }
};
void fillSmoothedState(PackUniformValues& values, const net::minecraft::entity::player::ClientPlayerEntity* player) {
 values.timeAngle = static_cast<float>(values.worldTime) / 24000.0f;
 values.timeBrightness = std::max(std::sin(values.timeAngle * kPiF * 2.0f), 0.0f);
 values.moonBrightness = std::max(std::sin(values.timeAngle * kPiF * -2.0f), 0.0f);
 const float adjTime = std::fabs(std::fmod(values.worldTime / 1000.0f + 6.0f, 24.0f) - 12.0f);
 values.day = std::clamp(5.4f - adjTime, 0.0f, 1.0f);
 values.night = std::clamp(adjTime - 6.0f, 0.0f, 1.0f);
 values.dawnDusk = (1.0f - values.day) - values.night;
 const float shadowAngleDegrees = values.shadowAngle * 360.0f;
 values.shadowFade = std::clamp(1.0f - (std::fabs(std::fabs(shadowAngleDegrees) - 0.25f) - 0.23f) * 100.0f,
                                0.0f, 1.0f);
 const float sunAngleDegrees = values.sunAngle * 360.0f;
 values.shdFade = std::clamp(1.0f - (std::fabs(std::fabs(sunAngleDegrees) - 0.25f) - 0.225f) * 40.0f, 0.0f,
                             1.0f);
 const int precipitation = values.biomePrecipitation;
 const float eyeY = player != nullptr ? static_cast<float>(player->y + player->getEyeHeight()) : 0.0f;
 const float rawEyeInCave = eyeY < 5.0f ? 1.0f - static_cast<float>(values.eyeBrightness[1]) / 240.0f : 0.0f;
 float velocity = 0.0f;
 float difSum = 0.0f;
 for(int axis = 0; axis < 3; ++axis) {
  const float d = values.cameraPosition[axis] - values.previousCameraPosition[axis];
  velocity += d * d;
  difSum += std::fabs(d);
 }
 velocity = std::sqrt(velocity);
 const float rawMoving = difSum > 0.0f && difSum < 1.0f ? 1.0f : 0.0f;
 static SmoothedState eyeInCave{6.0f, 12.0f};
 static SmoothedState rainS{15.0f, 15.0f};
 static SmoothedState rainShining{10.0f, 11.0f};
 static SmoothedState rainS2{70.0f, 1.0f};
 static SmoothedState dry{20.0f, 10.0f};
 static SmoothedState rainy{20.0f, 10.0f};
 static SmoothedState snowy{20.0f, 10.0f};
 static SmoothedState starterInner{0.0f, 31536000.0f};
 static SmoothedState starterOuter{20.0f, 20.0f};
 static SmoothedState frameTimeS{5.0f, 5.0f};
 static SmoothedState eyeBrightnessMS{5.0f, 5.0f};
 static SmoothedState precipitationRain{6.0f, 6.0f};
 static SmoothedState hurtSmooth{0.0f, 0.1f};
 static SmoothedState sneakS{2.0f, 0.9f};
 static SmoothedState burningS{1.0f, 2.0f};
 static SmoothedState speedS{1.0f, 1.5f};
 eyeInCave.update(rawEyeInCave, values.frameTime);
 rainS.update(values.rainStrength, values.frameTime);
 rainShining.update(values.rainStrength, values.frameTime);
 rainS2.update(values.rainStrength, values.frameTime);
 dry.update(precipitation == 0 ? 1.0f : 0.0f, values.frameTime);
 rainy.update(precipitation == 1 ? 1.0f : 0.0f, values.frameTime);
 snowy.update(precipitation == 2 ? 1.0f : 0.0f, values.frameTime);
 starterInner.update(rawMoving, values.frameTime);
 starterOuter.update(starterInner.value, values.frameTime);
 frameTimeS.update(values.frameTime, values.frameTime);
 eyeBrightnessMS.update(static_cast<float>(values.eyeBrightness[1]) / 240.0f, values.frameTime);
 precipitationRain.update(precipitation == 1 && values.cameraPosition[1] < 96.0f ? 1.0f : 0.0f,
                          values.frameTime);
 const float hurtFactor =
     player != nullptr && (player->hurtTime > 0 || player->deathTime > 0) ? 0.4f : 0.0f;
 hurtSmooth.update(hurtFactor, values.frameTime);
 const float sneakFactor = player != nullptr && player->isSneaking() ? 1.0f : 0.0f;
 sneakS.update(sneakFactor, values.frameTime);
 const float burnFactor = player != nullptr && player->isOnFire() ? 1.0f : 0.0f;
 burningS.update(burnFactor, values.frameTime);
 speedS.update(velocity / values.frameTime, values.frameTime);
 values.rainStrengthS = rainS.value;
 values.rainStrengthShiningStars = rainShining.value;
 values.rainStrengthS2 = rainS2.value;
 values.rainFactor = rainS.value;
 values.isDry = dry.value;
 values.isRainy = rainy.value;
 values.isSnowy = snowy.value;
 values.isEyeInCave = values.isEyeInWater == 0 ? eyeInCave.value : 0.0f;
 values.velocity = velocity;
 values.starter = starterOuter.value;
 values.frameTimeSmooth = frameTimeS.value;
 values.eyeBrightnessM = eyeBrightnessMS.value;
 values.biomeTemp = values.temperature;
 values.isPrecipitationRain = precipitationRain.value;
 values.touchmybody = hurtSmooth.value;
 values.sneakSmooth = sneakS.value;
 values.burningSmooth = burningS.value;
 values.effectStrength = 1.0f - std::exp(-speedS.value * 0.003906f);
 values.blindFactor = 0.0f;
 values.pi = kPiF;
 values.anisotropicFiltering = 0;
 values.heavyFog = 0;
}
void fillClientAndPlayerState(PackUniformValues& values,
                              const net::minecraft::client::Minecraft* minecraft,
                              const net::minecraft::World* world,
                              const FrameRenderCamera& camera,
                              float tickDelta,
                              float eyeBrightnessHalflife) {
 if(minecraft != nullptr) {
  values.firstPersonCamera = minecraft->options.thirdPerson ? 0 : 1;
  values.currentColorSpace = 0;
  if(minecraft->gameRenderer != nullptr) {
   if(minecraft->gameRenderer->packDefinition().supportsColorCorrection) {
    values.currentColorSpace = std::clamp(minecraft->options.colorSpace, 0, 4);
   }
  } else {
   values.currentColorSpace = std::clamp(minecraft->options.colorSpace, 0, 4);
  }
  values.screenBrightness = minecraft->options.brightness;
  values.hideGUI = minecraft->options.hideHud ? 1 : 0;
  values.textureFilteringMode = minecraft->options.mipmapLinear ? 1 : 0;
  values.isRightHanded = 1;
  if(minecraft->crosshairTarget.has_value() && minecraft->crosshairTarget->type == HitResultType::BLOCK &&
     world != nullptr) {
   const HitResult& hit = *minecraft->crosshairTarget;
   const int blockId = world->getBlockId(hit.blockX, hit.blockY, hit.blockZ);
   std::string selectedName =
       blockId > 0 && blockId < Block::BLOCK_COUNT && Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr
           ? Block::BLOCKS[static_cast<std::size_t>(blockId)]->getTranslationKey()
           : std::string{};
   if(selectedName.rfind("tile.", 0) == 0) selectedName.erase(0, 5);
   values.currentSelectedBlockId = render::resolveShaderObjectId(
       "block", PackCatalog::lower(std::move(selectedName)), blockId);
   values.currentSelectedBlockPos[0] = static_cast<float>(static_cast<double>(hit.blockX) + 0.5 - camera.eyeX);
   values.currentSelectedBlockPos[1] = static_cast<float>(static_cast<double>(hit.blockY) + 0.5 - camera.eyeY);
   values.currentSelectedBlockPos[2] = static_cast<float>(static_cast<double>(hit.blockZ) + 0.5 - camera.eyeZ);
  }
 }
 const net::minecraft::entity::player::ClientPlayerEntity* player =
     minecraft != nullptr ? minecraft->player : nullptr;
 if(player != nullptr) {
  values.maxPlayerHealth = static_cast<float>(std::max(1, player->maxHealth));
  values.currentPlayerHealth = std::clamp(static_cast<float>(player->health) / values.maxPlayerHealth, 0.0f, 1.0f);
  values.maxPlayerAir = static_cast<float>(std::max(1, player->maxAir));
  values.currentPlayerAir = std::clamp(static_cast<float>(player->air) / values.maxPlayerAir, 0.0f, 1.0f);
  values.maxPlayerArmor = 50.0f;
  values.currentPlayerArmor =
      std::clamp(static_cast<float>(player->inventory.getTotalArmorDurability()) / values.maxPlayerArmor, 0.0f, 1.0f);
  values.currentPlayerHunger = 1.0f;
  values.maxPlayerHunger = 20.0f;
  values.eyePosition[0] = static_cast<float>(player->x);
  values.eyePosition[1] = static_cast<float>(player->y + player->getEyeHeight());
  values.eyePosition[2] = static_cast<float>(player->z);
  values.eyeAltitude = values.cameraPosition[1];
  if(player->isInFluid(net::minecraft::block::material::Material::WATER))
   values.isEyeInWater = 1;
  else if(player->isInFluid(net::minecraft::block::material::Material::LAVA))
   values.isEyeInWater = 2;
  else
   values.isEyeInWater = 0;
  values.feetInWater =
      world != nullptr &&
              world->isMaterialInBox(player->boundingBox.contract(0.001, 0.0, 0.001).offset(0.0, -0.05, 0.0),
                                     net::minecraft::block::material::Material::WATER)
          ? 1
          : 0;
  if(const ItemStack* held = player->inventory.getSelectedItem()) {
   values.heldItemId = held->itemId;
   if(held->itemId >= 0 && held->itemId < net::minecraft::block::Block::BLOCK_COUNT &&
      net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(held->itemId)] != nullptr)
    values.heldBlockLightValue =
        net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(held->itemId)]->emission();
  }
  values.heldBlockLightValue2 = 0;
  if(minecraft != nullptr && minecraft->gameRenderer != nullptr &&
     minecraft->gameRenderer->packDefinition().oldHandLight) {
   values.heldBlockLightValue = std::max(values.heldBlockLightValue, values.heldBlockLightValue2);
  }
  values.handLightPackedLR = static_cast<int>(
      (static_cast<std::uint32_t>(values.heldBlockLightValue2) & 0xFFFFu) |
      ((static_cast<std::uint32_t>(values.heldBlockLightValue) & 0xFFFFu) << 16));
  for(int axis = 0; axis < 3; ++axis) {
   values.relativeEyePosition[axis] =
       static_cast<float>(g_cameraTracker.currentUnshifted(axis)) - values.eyePosition[axis];
  }
  const Vec3d look = player->getLookVector(tickDelta);
  values.playerLookVector[0] = static_cast<float>(look.x);
  values.playerLookVector[1] = static_cast<float>(look.y);
  values.playerLookVector[2] = static_cast<float>(look.z);
  const float body = player->bodyYaw * 0.01745329252f;
  values.playerBodyVector[0] = -std::sin(body);
  values.playerBodyVector[1] = 0.0f;
  values.playerBodyVector[2] = std::cos(body);
  values.isSneaking = player->isSneaking() ? 1 : 0;
  values.isHurt = player->hurtTime > 0 ? 1 : 0;
  values.isBurning = player->isOnFire() ? 1 : 0;
  values.isOnGround = player->onGround ? 1 : 0;
  values.isInvisible = player->getFlag(5) ? 1 : 0;
  values.isRiding = player->vehicle != nullptr ? 1 : 0;
  if(player->vehicle != nullptr) {
   net::minecraft::Entity* vehicle = player->vehicle;
   values.vehicleId =
       render::resolveShaderObjectId("entity", ::net::minecraft::entity::EntityRegistry::getId(*vehicle), -1);
   const float vehicleYaw = vehicle->yaw * 0.01745329252f;
   const float vehiclePitch = vehicle->pitch * 0.01745329252f;
   values.vehicleLookVector[0] = -std::sin(vehicleYaw) * std::cos(vehiclePitch);
   values.vehicleLookVector[1] = -std::sin(vehiclePitch);
   values.vehicleLookVector[2] = std::cos(vehicleYaw) * std::cos(vehiclePitch);
   values.relativeVehiclePosition[0] = static_cast<float>(camera.eyeX - vehicle->x);
   values.relativeVehiclePosition[1] = static_cast<float>(camera.eyeY - vehicle->y);
   values.relativeVehiclePosition[2] = static_cast<float>(camera.eyeZ - vehicle->z);
   if(dynamic_cast<::net::minecraft::entity::vehicle::BoatEntity*>(vehicle) != nullptr && world != nullptr) {
    values.vehicleInWater =
        world->isMaterialInBox(vehicle->boundingBox.expand(0.0, -0.001, 0.0),
                               net::minecraft::block::material::Material::WATER)
            ? 1
            : 0;
   }
  }
  if(world != nullptr) {
   const int bx = MathHelper::floor(static_cast<float>(player->x));
   const int by = MathHelper::floor(static_cast<float>(player->y + player->getEyeHeight()));
   const int bz = MathHelper::floor(static_cast<float>(player->z));
   values.eyeBrightness[0] = world->getBrightness(net::minecraft::LightType::Block, bx, by, bz) * 16;
   values.eyeBrightness[1] = world->getBrightness(net::minecraft::LightType::Sky, bx, by, bz) * 16;
   static float smoothBlock = 0.0f;
   static bool smoothBlockInitialized = false;
   static float smoothSky = 0.0f;
   static bool smoothSkyInitialized = false;
   const float halfLifeSeconds = std::max(0.001f, eyeBrightnessHalflife * 0.1f);
   values.eyeBrightnessSmooth[0] = static_cast<int>(smoothExponential(
       static_cast<float>(values.eyeBrightness[0]), smoothBlock, smoothBlockInitialized, values.frameTime,
       halfLifeSeconds));
   values.eyeBrightnessSmooth[1] = static_cast<int>(smoothExponential(
       static_cast<float>(values.eyeBrightness[1]), smoothSky, smoothSkyInitialized, values.frameTime,
       halfLifeSeconds));
  }
  fillSmoothedState(values, player);
 }
}
} // namespace
PackUniformValues buildShaderFrameData(int width,
                                       int height,
                                       float worldTime,
                                       int shadowMapResolution,
                                       bool normalAvailable,
                                       bool shadowAvailable,
                                       const render::FrameRenderCamera& camera,
                                       const render::FrameRenderCamera& shadowCamera,
                                       const net::minecraft::World* world,
                                       float eyeBrightnessHalflife) {
 PackUniformValues values;
 const bool firstFrame = !g_frameState.initialized;
 tickFrameClock(values);
 fillViewport(values, width, height);
 fillCameraPlanes(values, camera);
 fillFog(values);
 fillFlags(values, shadowMapResolution, normalAvailable, shadowAvailable);
 fillCameraState(values, camera, shadowCamera, shadowAvailable);
 fillSystemTime(values);
 const float tickDelta = worldTime - std::floor(worldTime);
 if(world != nullptr) {
  fillWorldState(values, *world, camera, tickDelta);
 }
 fillClientAndPlayerState(values, net::minecraft::client::Minecraft::INSTANCE, world, camera, tickDelta,
                          eyeBrightnessHalflife);
 values.centerDepthSmooth = g_centerDepthSmooth;
 if(firstFrame) {
  std::fill(std::begin(values.gbufferPreviousProjection), std::end(values.gbufferPreviousProjection), 0.0f);
  std::fill(std::begin(values.gbufferPreviousModelView), std::end(values.gbufferPreviousModelView), 0.0f);
  for(int index : {0, 5, 10, 15}) {
   values.gbufferPreviousProjection[index] = 1.0f;
   values.gbufferPreviousModelView[index] = 1.0f;
  }
 }
 g_frameState.current = values;
 g_frameState.initialized = true;
 return values;
}
float smoothExponential(float target, float& accumulator, bool& initialized, float frameTime,
                        float halfLifeSeconds) {
 if(!initialized) {
  accumulator = target;
  initialized = true;
  return accumulator;
 }
 if(halfLifeSeconds <= 0.0f) {
  accumulator = target;
  return accumulator;
 }
 const float alpha = 1.0f - std::exp(-std::max(0.0f, frameTime) * 0.693147f / halfLifeSeconds);
 accumulator += (target - accumulator) * std::clamp(alpha, 0.0f, 1.0f);
 return accumulator;
}
float updateCenterDepthSmooth(float windowDepth01, float nearPlane, float farPlane, float frameTime,
                              float halfLife) {
 const float z = std::clamp(windowDepth01, 0.0f, 1.0f);
 const float n = std::max(nearPlane, 1e-4f);
 const float f = std::max(farPlane, n + 1e-3f);
 const float linear = (n * f) / std::max(1e-6f, f + z * (n - f));
 const float halfLifeSeconds = std::max(0.001f, halfLife * 0.1f);
 const float alpha = 1.0f - std::exp(-std::max(0.0f, frameTime) * 0.693147f / halfLifeSeconds);
 g_centerDepthSmooth += (linear - g_centerDepthSmooth) * std::clamp(alpha, 0.0f, 1.0f);
 return g_centerDepthSmooth;
}
float updateWetnessSmooth(float rainStrength, float frameTime, float wetnessHalflife, float drynessHalflife) {
 static float accumulator = 0.0f;
 static bool initialized = false;
 const float target = std::clamp(rainStrength, 0.0f, 1.0f);
 smoothExponential(target, accumulator, initialized, frameTime,
                   (target > accumulator ? wetnessHalflife : drynessHalflife) * 0.1f);
 g_wetnessSmooth = accumulator;
 return g_wetnessSmooth;
}
} // namespace net::minecraft::client::render
