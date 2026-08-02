#include "net/minecraft/client/render/uniforms/FrameData.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <cstring>
#include <limits>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/pipeline/Manager.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/entity/LightningEntity.hpp"
#include "net/minecraft/entity/EntityRegistry.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/vehicle/BoatEntity.hpp"
#include "net/minecraft/util/hit/HitResult.hpp"
#include "net/minecraft/util/hit/HitResultType.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/light/LightType.hpp"
namespace net::minecraft::client::render {
float g_centerDepthSmooth = 0.0f;
float g_wetnessSmooth = 0.0f;
namespace {
const auto g_shaderClockStart = std::chrono::steady_clock::now();
void column(float* m, int c, float x, float y, float z, float w) {
 m[c * 4] = x;
 m[c * 4 + 1] = y;
 m[c * 4 + 2] = z;
 m[c * 4 + 3] = w;
}
void inverse(const float* source, float* destination) {
 net::minecraft::util::math::Matrix4f matrix;
 matrix.set(source);
 matrix.invert();
 std::memcpy(destination, matrix.data(), sizeof(float) * 16);
}
void shadowModelView(float* m, const render::FrameRenderCamera& shadow, const render::FrameRenderCamera& camera) {
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowMatrices.java
 if(shadow.hasExplicitModelView) {
  std::memcpy(m, shadow.explicitModelView, sizeof(float) * 16);
  return;
 }
 const float right[3] = {shadow.viewRightX, shadow.viewRightY, shadow.viewRightZ}, up[3] = {shadow.viewUpX, shadow.viewUpY, shadow.viewUpZ}, forward[3] = {-shadow.viewForwardX, -shadow.viewForwardY, -shadow.viewForwardZ};
 const float delta[3] = {static_cast<float>(camera.eyeX - shadow.eyeX), static_cast<float>(camera.eyeY - shadow.eyeY), static_cast<float>(camera.eyeZ - shadow.eyeZ)};
 column(m, 0, right[0], up[0], forward[0], 0.0f);
 column(m, 1, right[1], up[1], forward[1], 0.0f);
 column(m, 2, right[2], up[2], forward[2], 0.0f);
 column(m, 3, right[0] * delta[0] + right[1] * delta[1] + right[2] * delta[2], up[0] * delta[0] + up[1] * delta[1] + up[2] * delta[2], forward[0] * delta[0] + forward[1] * delta[1] + forward[2] * delta[2], 1.0f);
}
void biomeData(const Biome& biome, PackUniformValues& values) {
 values.biome = static_cast<int>(biome.id);
 values.biomePrecipitation = biome.canSnow() ? 2 : biome.canRain() ? 1
                                                                   : 0;
 switch(biome.id) {
 case BiomeId::Rainforest:
  values.biomeCategory = 3;
  values.temperature = 0.95f;
  values.rainfall = 0.9f;
  break;
 case BiomeId::Swampland:
  values.biomeCategory = 14;
  values.temperature = 0.8f;
  values.rainfall = 0.9f;
  break;
 case BiomeId::SeasonalForest:
 case BiomeId::Forest:
  values.biomeCategory = 10;
  values.temperature = 0.7f;
  values.rainfall = 0.8f;
  break;
  case BiomeId::Savanna:
  case BiomeId::Shrubland:
   // Java 26.1 BiomeCategories: SAVANNA = 6 (BiomeCategories.java); plains stays 5.
   values.biomeCategory = 6;
   values.temperature = 0.8f;
   values.rainfall = 0.4f;
   break;
  case BiomeId::Plains:
   values.biomeCategory = 5;
   values.temperature = 0.8f;
   values.rainfall = 0.4f;
   break;
 case BiomeId::Taiga:
  values.biomeCategory = 1;
  values.temperature = 0.25f;
  values.rainfall = 0.8f;
  break;
 case BiomeId::Desert:
 case BiomeId::IceDesert:
  values.biomeCategory = 12;
  values.temperature = biome.id == BiomeId::Desert ? 2.0f : 0.0f;
  values.rainfall = 0.0f;
  break;
 case BiomeId::Tundra:
  values.biomeCategory = 7;
  values.temperature = 0.0f;
  values.rainfall = 0.5f;
  break;
 case BiomeId::Hell:
  values.biomeCategory = 16;
  values.temperature = 2.0f;
  values.rainfall = 0.0f;
  break;
 case BiomeId::Sky:
  values.biomeCategory = 8;
  values.temperature = 0.5f;
  values.rainfall = 0.0f;
  break;
 }
}
void systemTime(PackUniformValues& values) {
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
constexpr float kPiF = 3.14159265358979323846f;
} // namespace
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/transforms/SmoothedFloat.java
// s_t = lerp(s_{t-1}, x_t, 1 - e^(-ln(2) * frameTime / halfLife)); the first value is used raw.
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
namespace {
// Rotates a world-space direction about the world Z axis by sunPathRotation degrees. Iris applies
// this as the RZ(sunPathRotation) term of the celestial matrix (CelestialUniforms.java); the beta
// celestial frame is that chain without the fixed RY(-90) renderSky prefix, so the rotation acts
// on the world direction directly.
void applySunPathRotation(const float in[3], float sunPathRotationDegrees, float out[3]) {
 if(sunPathRotationDegrees == 0.0f) {
  out[0] = in[0];
  out[1] = in[1];
  out[2] = in[2];
  return;
 }
 const float radians = sunPathRotationDegrees * (kPiF / 180.0f);
 const float cs = std::cos(radians);
 const float sn = std::sin(radians);
 out[0] = in[0] * cs - in[1] * sn;
 out[1] = in[0] * sn + in[1] * cs;
 out[2] = in[2];
}
} // namespace
PackUniformValues buildShaderFrameData(int width, int height, float farPlane, float worldTime, int shadowMapResolution, bool normalAvailable, bool shadowAvailable, const render::FrameRenderCamera& camera, const render::FrameRenderCamera& shadowCamera, const net::minecraft::World* world, float eyeBrightnessHalflife) {
  static PackUniformValues previousFrame;
  static PackUniformValues currentFrame;
  static bool initialized = false;
  static int frameCounter = 0;
  static float frameTimeCounterAccumulator = 0.0f;
  static auto previousFrameTime = g_shaderClockStart;
 PackUniformValues values;
 const bool firstFrame = !initialized;
 const auto now = std::chrono::steady_clock::now();
 if(initialized) {
  previousFrame = currentFrame;
  // Java SystemTimeUniforms.Timer: frame time is truncated to millisecond resolution.
  const float elapsed = std::chrono::duration<float>(now - previousFrameTime).count();
  values.frameTime = std::max(0.0f, std::floor(elapsed * 1000.0f) / 1000.0f);
 } else {
  values.frameTime = 0.0f;
 }
 values.frameCounter = frameCounter;
 frameCounter = (frameCounter + 1) % 720720;
 previousFrameTime = now;
 // Java Timer.beginFrame: accumulate lastFrameTime and reset to zero once it reaches an hour.
 frameTimeCounterAccumulator += values.frameTime;
 if(frameTimeCounterAccumulator >= 3600.0f) {
  frameTimeCounterAccumulator = 0.0f;
 }
 values.frameTimeCounter = frameTimeCounterAccumulator;
  values.viewWidth = static_cast<float>(width);
  values.viewHeight = static_cast<float>(height);
  values.aspectRatio = values.viewWidth / std::max(values.viewHeight, 1.0f);
  values.nearPlane = camera.perspectiveNear;
  values.farPlane = farPlane;
  const float tickDelta = worldTime - std::floor(worldTime);
  {
   const auto& fog = render::core::fog();
   values.fogColor[0] = fog.color[0];
   values.fogColor[1] = fog.color[1];
   values.fogColor[2] = fog.color[2];
   values.fogDensity = fog.density;
   values.fogStart = fog.start;
   values.fogEnd = fog.end;
    values.fogMode = fog.enabled ? render::core::fogModeToGlConstant(fog.mode) : 0;
    // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/FogUniforms.java
    values.fogShape = fog.enabled ? 1 : -1;
   // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/IrisInternalUniforms.java
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
  values.normalAvailable = normalAvailable ? 1 : 0;
  values.shadowAvailable = shadowAvailable ? 1 : 0;
  values.shadowMapResolution = static_cast<float>(shadowMapResolution);
  // Java CameraUniforms.CameraPositionTracker: cameraPosition is the shifted position,
  // cameraPositionInt/Fract the unshifted one (CameraUniforms.java:31-34); previous*
  // mirror last frame's state. The tracker is updated here, after the frame-time
  // bookkeeping, so the first frame's previous* are zeros exactly like Java's
  // notifier-driven update.
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CameraUniforms.java
  static CameraPositionTracker cameraTracker;
  cameraTracker.update(camera.eyeX, camera.eyeY, camera.eyeZ);
  for(int axis = 0; axis < 3; ++axis) {
   values.cameraPosition[axis] = static_cast<float>(cameraTracker.current(axis));
  }
  splitCameraPosition(cameraTracker.currentUnshifted(0), cameraTracker.currentUnshifted(1),
                      cameraTracker.currentUnshifted(2), values.cameraPositionInt,
                      values.cameraPositionFract);
  for(int axis = 0; axis < 3; ++axis) {
   values.previousCameraPosition[axis] = static_cast<float>(cameraTracker.previous(axis));
  }
  splitCameraPosition(cameraTracker.previousUnshifted(0), cameraTracker.previousUnshifted(1),
                      cameraTracker.previousUnshifted(2), values.previousCameraPositionInt,
                      values.previousCameraPositionFract);
  render::buildCameraProjection(values.gbufferProjection, camera, farPlane);
 render::buildCameraProjectionInverse(values.gbufferProjectionInverse, camera, farPlane);
 buildCameraModelView(values.gbufferModelView, camera);
 buildCameraModelViewInverse(values.gbufferModelViewInverse, camera);
 std::copy(std::begin(previousFrame.gbufferProjection), std::end(previousFrame.gbufferProjection), values.gbufferPreviousProjection);
 std::copy(std::begin(previousFrame.gbufferModelView), std::end(previousFrame.gbufferModelView), values.gbufferPreviousModelView);
 render::directionToView(0.0f, 1.0f, 0.0f, camera, values.upPosition);
 if(shadowAvailable) {
  shadowModelView(values.shadowModelView, shadowCamera, camera);
  render::FrameRenderCamera shadowProjCamera = shadowCamera;
  if(!shadowCamera.orthographic) {
   const float nearZ = std::max(0.05f, shadowCamera.perspectiveNear);
   const float farZ = shadowCamera.perspectiveFar > nearZ ? shadowCamera.perspectiveFar : nearZ + 1.0f;
   shadowProjCamera.perspectiveFar = farZ;
  }
  render::buildCameraProjection(values.shadowProjection, shadowProjCamera, farPlane);
  render::buildCameraProjectionInverse(values.shadowProjectionInverse, shadowProjCamera, farPlane);
  inverse(values.shadowModelView, values.shadowModelViewInverse);
 }
 systemTime(values);
  if(world != nullptr) {
   const auto& sun = world->lightRegistry().sun();
   const net::minecraft::client::Minecraft* celestialClient = net::minecraft::client::Minecraft::INSTANCE;
   const PackDefinition* activeDef =
       celestialClient != nullptr && celestialClient->gameRenderer != nullptr &&
               celestialClient->gameRenderer->shaderPacks() != nullptr
           ? celestialClient->gameRenderer->shaderPacks()->activeDefinition()
           : nullptr;
   const float sunPathRotation = activeDef != nullptr ? activeDef->sunPathRotation : 0.0f;
   // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CelestialUniforms.java
   float sunDirection[3] = {sun.directionX, sun.directionY, sun.directionZ};
   applySunPathRotation(sunDirection, sunPathRotation, sunDirection);
   render::directionToView(sunDirection[0], sunDirection[1], sunDirection[2], camera, values.sunPosition);
   render::directionToView(-sunDirection[0], -sunDirection[1], -sunDirection[2], camera, values.moonPosition);
   auto scaleTo100 = [](float v[3]) {
    const float len = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if(len > 1e-6f) {
     const float s = 100.0f / len;
     v[0] *= s; v[1] *= s; v[2] *= s;
    }
   };
   scaleTo100(values.sunPosition);
   scaleTo100(values.moonPosition);
   
   values.sunColor[0] = sun.red;
   values.sunColor[1] = sun.green;
   values.sunColor[2] = sun.blue;
   values.sunIntensity = sun.intensity;
   const std::uint64_t absoluteTime = world->getTime();
   
   // Pin worldTime when clientTimeMode is Day (1) or Night (2).
   if(world->clientTimeMode() == 1) {
    values.worldDay = static_cast<int>(absoluteTime / 24000ULL);
    values.worldTime = 6000;
   } else if(world->clientTimeMode() == 2) {
    values.worldDay = static_cast<int>(absoluteTime / 24000ULL);
    values.worldTime = 18000;
   } else {
    values.worldTime = static_cast<int>(absoluteTime % 24000ULL);
    values.worldDay = static_cast<int>(absoluteTime / 24000ULL);
   }
   
    values.moonPhase = static_cast<int>((absoluteTime / 24000ULL) % 8ULL);
    const float celestialAngle = world->getTime(tickDelta);
    // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CelestialUniforms.java
    values.sunAngle = celestialSunAngle(celestialAngle);
    values.shadowAngle = shadowAngleFromCelestial(celestialAngle);
    const float sunAngleDegrees = values.sunAngle * 360.0f;
   // Java getShadowLightPosition: end flash in the end when the pack opts in, else the sun by
   // day and the moon at night.
   const bool isEnd = world->dimension != nullptr && world->dimension->id == 1;
   if(isEnd && activeDef != nullptr && activeDef->endFlashShadows) {
    std::copy(std::begin(values.endFlashPosition), std::end(values.endFlashPosition), values.shadowLightPosition);
   } else if(sunAngleDegrees < 180.0f) {
    std::copy(std::begin(values.sunPosition), std::end(values.sunPosition), values.shadowLightPosition);
   } else {
    std::copy(std::begin(values.moonPosition), std::end(values.moonPosition), values.shadowLightPosition);
   }
  
  values.rainStrength = world->getRainGradient(tickDelta);
  // Wetness EMA applied in PackManager::setFrameUniforms with pack halflife.
  values.wetness = g_wetnessSmooth;
  values.thunderStrength = world->getThunderGradient(tickDelta);
  {
   // ShaderDoc skyColor = clear/sky RGB (World::getSkyColor → world_color Lua).
   const Vec3d sky = world->getSkyColor(
       net::minecraft::client::Minecraft::INSTANCE != nullptr
           ? net::minecraft::client::Minecraft::INSTANCE->camera
           : nullptr,
       tickDelta);
   values.skyColor[0] = static_cast<float>(sky.x);
   values.skyColor[1] = static_cast<float>(sky.y);
   values.skyColor[2] = static_cast<float>(sky.z);
  }
  if(world->dimension != nullptr) {
   values.hasCeiling = world->dimension->hasCeiling ? 1 : 0;
   values.hasSkylight = world->dimension->hasCeiling ? 0 : 1;
   values.ambientLight = world->dimension->isNether ? 0.1f : 0.0f;
   values.bedrockLevel = 0;
   values.heightLimit = 128;
   values.logicalHeightLimit = 128;
   values.cloudHeight = world->dimension->hasCeiling
                            ? std::numeric_limits<float>::quiet_NaN()
                            : world->dimension->getCloudHeight();
   // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/IrisExclusiveUniforms.java
   values.seaLevel = world->dimension->isNether ? 32 : isEnd ? 0
                                                             : 63;
  }
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/mixin/MixinLevelRenderer.java
  values.cloudTime = static_cast<float>(static_cast<double>(absoluteTime % 102400ULL) + tickDelta) * 0.03f;
  biomeData(world->getBiome(static_cast<int>(camera.eyeX), static_cast<int>(camera.eyeZ)), values);
  for(Entity* entity : world->globalEntities) {
   if(dynamic_cast<LightningEntity*>(entity) == nullptr) continue;
   // Java: bolt position relative to the unshifted camera position.
   values.lightningBoltPosition[0] = static_cast<float>(entity->x - camera.eyeX);
   values.lightningBoltPosition[1] = static_cast<float>(entity->y - camera.eyeY);
   values.lightningBoltPosition[2] = static_cast<float>(entity->z - camera.eyeZ);
   values.lightningBoltPosition[3] = 1.0f;
   break;
  }
  values.entityId = render::core::entityId();
  {
   // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/IrisInternalUniforms.java
   for(int c = 0; c < 3; ++c) {
    for(int r = 0; r < 3; ++r) {
     values.irisDefaultNormalMat[c * 3 + r] = values.gbufferModelViewInverse[r * 4 + c];
    }
   }
  }
 }
 const net::minecraft::client::Minecraft* minecraft = net::minecraft::client::Minecraft::INSTANCE;
 const net::minecraft::entity::player::ClientPlayerEntity* player = minecraft != nullptr ? minecraft->player : nullptr;
 if(minecraft != nullptr) {
  values.firstPersonCamera = minecraft->options.thirdPerson ? 0 : 1;
  values.currentColorSpace = 0;
  if(const auto* def =
         minecraft->gameRenderer != nullptr && minecraft->gameRenderer->shaderPacks() != nullptr
             ? minecraft->gameRenderer->shaderPacks()->activeDefinition()
             : nullptr) {
   if(def->supportsColorCorrection) {
    values.currentColorSpace = std::clamp(minecraft->options.colorSpace, 0, 4);
   }
  } else {
   values.currentColorSpace = std::clamp(minecraft->options.colorSpace, 0, 4);
  }
  values.screenBrightness = minecraft->options.brightness;
  values.hideGUI = minecraft->options.hideHud ? 1 : 0;
  values.textureFilteringMode = minecraft->options.mipmapLinear ? 1 : 0;
  values.isRightHanded = 1;
  if(minecraft->crosshairTarget.has_value() &&
     minecraft->crosshairTarget->type == HitResultType::BLOCK && world != nullptr) {
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
  if(player != nullptr) {
   values.maxPlayerHealth = static_cast<float>(std::max(1, player->maxHealth));
   values.currentPlayerHealth = std::clamp(static_cast<float>(player->health) / values.maxPlayerHealth, 0.0f, 1.0f);
   values.maxPlayerAir = static_cast<float>(std::max(1, player->maxAir));
   values.currentPlayerAir = std::clamp(static_cast<float>(player->air) / values.maxPlayerAir, 0.0f, 1.0f);
   // Java IrisExclusiveUniforms: armor is divided by 50 and maxPlayerArmor is fixed at 50.
   values.maxPlayerArmor = 50.0f;
   values.currentPlayerArmor = std::clamp(static_cast<float>(player->inventory.getTotalArmorDurability()) / values.maxPlayerArmor, 0.0f, 1.0f);
   values.currentPlayerHunger = 1.0f;
   values.maxPlayerHunger = 20.0f;
   // Java IrisExclusiveUniforms.getEyePosition: unshifted (raw world) eye position.
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
  if(minecraft != nullptr && minecraft->gameRenderer != nullptr && minecraft->gameRenderer->shaderPacks() != nullptr) {
   if(const auto* def = minecraft->gameRenderer->shaderPacks()->activeDefinition();
      def != nullptr && def->oldHandLight) {
    values.heldBlockLightValue = std::max(values.heldBlockLightValue, values.heldBlockLightValue2);
   }
  }
  values.handLightPackedLR = static_cast<int>(
      (static_cast<std::uint32_t>(values.heldBlockLightValue2) & 0xFFFFu) |
      ((static_cast<std::uint32_t>(values.heldBlockLightValue) & 0xFFFFu) << 16));
  // Java IrisExclusiveUniforms.java:81: relativeEyePosition =
  // getUnshiftedCameraPosition() - eye position (both unshifted).
  for(int axis = 0; axis < 3; ++axis) {
   values.relativeEyePosition[axis] =
       static_cast<float>(cameraTracker.currentUnshifted(axis)) - values.eyePosition[axis];
  }
  // Java CommonUniforms.playerLookVector = getViewVector(tickDelta) — yaw/pitch interpolated.
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
   Entity* vehicle = player->vehicle;
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
   // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CommonUniforms.java
   // SmoothedVec2f over getEyeBrightness; the half life directive is in deciseconds.
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
  {
   // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/HardcodedCustomUniforms.java
   // Java SmoothedFloat instances carry separate up/down half lives in deciseconds.
   struct SmoothedState {
    float up = 0.0f;
    float down = 0.0f;
    float value = 0.0f;
    bool initialized = false;
    void update(float target, float frameTime) {
     smoothExponential(target, value, initialized, frameTime, (target > value ? up : down) * 0.1f);
    }
   };
   values.timeAngle = static_cast<float>(values.worldTime) / 24000.0f;
   values.timeBrightness = std::max(std::sin(values.timeAngle * kPiF * 2.0f), 0.0f);
   values.moonBrightness = std::max(std::sin(values.timeAngle * kPiF * -2.0f), 0.0f);
   const float adjTime = std::fabs(std::fmod(values.worldTime / 1000.0f + 6.0f, 24.0f) - 12.0f);
   values.day = std::clamp(5.4f - adjTime, 0.0f, 1.0f);
   values.night = std::clamp(adjTime - 6.0f, 0.0f, 1.0f);
   values.dawnDusk = (1.0f - values.day) - values.night;
   // Java feeds un-normalized degrees into these; the formula collapses to zero for every valid
   // angle, so replicate it verbatim (results stay identical to Iris 26.1).
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
   // Java divides by the last frame time (0 on the first frame, giving NaN exactly like Iris).
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
   // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CommonUniforms.java
   values.blindFactor = 0.0f;
   values.pi = kPiF;
   values.dhFarPlane = 0.01f;
   values.dhNearPlane = 0.01f;
   values.dhRenderDistance = static_cast<int>(values.farPlane / 16.0f);
   // Beta 1.7.3 has no anisotropic filtering, boss-bar fog or end flash: constants.
   values.anisotropicFiltering = 0;
   values.heavyFog = 0;
  }
 }
 {
  // Populated by PackManager::sampleCenterDepth (real center depthtex0 read).
  values.centerDepthSmooth = g_centerDepthSmooth;
  }
  if(firstFrame) {
   std::fill(std::begin(values.gbufferPreviousProjection), std::end(values.gbufferPreviousProjection), 0.0f);
   std::fill(std::begin(values.gbufferPreviousModelView), std::end(values.gbufferPreviousModelView), 0.0f);
   for(int index : {0, 5, 10, 15}) {
    values.gbufferPreviousProjection[index] = 1.0f;
    values.gbufferPreviousModelView[index] = 1.0f;
   }
  }
 currentFrame = values;
 initialized = true;
 return values;
}
float updateCenterDepthSmooth(float windowDepth01, float nearPlane, float farPlane, float frameTime,
                              float halfLife) {
 const float z = std::clamp(windowDepth01, 0.0f, 1.0f);
 const float n = std::max(nearPlane, 1e-4f);
 const float f = std::max(farPlane, n + 1e-3f);
 const float linear = (n * f) / std::max(1e-6f, f + z * (n - f));
 // Java CenterDepthSampler: decay = 1 / ((halfLife * 0.1) / LN2) — halfLife is in deciseconds.
 const float halfLifeSeconds = std::max(0.001f, halfLife * 0.1f);
 const float alpha = 1.0f - std::exp(-std::max(0.0f, frameTime) * 0.693147f / halfLifeSeconds);
 g_centerDepthSmooth += (linear - g_centerDepthSmooth) * std::clamp(alpha, 0.0f, 1.0f);
 return g_centerDepthSmooth;
}
float updateWetnessSmooth(float rainStrength, float frameTime, float wetnessHalflife, float drynessHalflife) {
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/CommonUniforms.java
 // Java: SmoothedFloat(directives.getWetnessHalfLife(), getDrynessHalfLife(), getRainStrength).
 static float accumulator = 0.0f;
 static bool initialized = false;
 const float target = std::clamp(rainStrength, 0.0f, 1.0f);
 smoothExponential(target, accumulator, initialized, frameTime,
                   (target > accumulator ? wetnessHalflife : drynessHalflife) * 0.1f);
 g_wetnessSmooth = accumulator;
 return g_wetnessSmooth;
}
} // namespace net::minecraft::client::render
