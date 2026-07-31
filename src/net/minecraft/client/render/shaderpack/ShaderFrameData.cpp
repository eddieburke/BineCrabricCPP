#include "net/minecraft/client/render/shaderpack/ShaderFrameData.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <cstring>
#include <limits>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackManager.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
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
namespace net::minecraft::client::render::shaderpack {
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
 const float right[3] = {shadow.viewRightX, shadow.viewRightY, shadow.viewRightZ}, up[3] = {shadow.viewUpX, shadow.viewUpY, shadow.viewUpZ}, forward[3] = {-shadow.viewForwardX, -shadow.viewForwardY, -shadow.viewForwardZ};
 const float delta[3] = {static_cast<float>(camera.eyeX - shadow.eyeX), static_cast<float>(camera.eyeY - shadow.eyeY), static_cast<float>(camera.eyeZ - shadow.eyeZ)};
 column(m, 0, right[0], up[0], forward[0], 0.0f);
 column(m, 1, right[1], up[1], forward[1], 0.0f);
 column(m, 2, right[2], up[2], forward[2], 0.0f);
 column(m, 3, right[0] * delta[0] + right[1] * delta[1] + right[2] * delta[2], up[0] * delta[0] + up[1] * delta[1] + up[2] * delta[2], forward[0] * delta[0] + forward[1] * delta[1] + forward[2] * delta[2], 1.0f);
}
void biomeData(const Biome& biome, ShaderUniformValues& values) {
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
void systemTime(ShaderUniformValues& values) {
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
double cameraOrigin(double value) {
 return value - std::fmod(value, 30000.0);
}
} // namespace
ShaderUniformValues buildShaderFrameData(int width, int height, float farPlane, float worldTime, int shadowMapResolution, bool normalAvailable, bool shadowAvailable, const render::FrameRenderCamera& camera, const render::FrameRenderCamera& shadowCamera, const net::minecraft::World* world, float eyeBrightnessHalflifeTicks) {
 static ShaderUniformValues previousFrame;
 static ShaderUniformValues currentFrame;
 static bool initialized = false;
 static int frameCounter = 0;
 static auto previousFrameTime = g_shaderClockStart;
 ShaderUniformValues values;
 const bool firstFrame = !initialized;
 const auto now = std::chrono::steady_clock::now();
 if(initialized) {
  previousFrame = currentFrame;
  values.frameTime = std::max(std::chrono::duration<float>(now - previousFrameTime).count(), 0.0f);
 } else {
  values.frameTime = 0.0f;
 }
 values.frameCounter = frameCounter;
 frameCounter = (frameCounter + 1) % 720720;
 previousFrameTime = now;
 values.frameTimeCounter =
     std::fmod(std::chrono::duration<float>(now - g_shaderClockStart).count(), 3600.0f);
 values.viewWidth = static_cast<float>(width);
 values.viewHeight = static_cast<float>(height);
 values.aspectRatio = values.viewWidth / std::max(values.viewHeight, 1.0f);
 values.nearPlane = camera.perspectiveNear;
 values.farPlane = farPlane;
 {
  const auto& fog = render::core::fog();
  values.fogColor[0] = fog.color[0];
  values.fogColor[1] = fog.color[1];
  values.fogColor[2] = fog.color[2];
  values.fogDensity = fog.density;
  values.fogStart = fog.start;
  values.fogEnd = fog.end;
  values.fogMode = fog.enabled ? fog.mode : 0;
  values.fogShape = fog.shape;
 }
 values.normalAvailable = normalAvailable ? 1 : 0;
 values.shadowAvailable = shadowAvailable ? 1 : 0;
 values.shadowMapResolution = static_cast<float>(shadowMapResolution);
 const double cameraOriginX = cameraOrigin(camera.eyeX);
 const double cameraOriginY = cameraOrigin(camera.eyeY);
 const double cameraOriginZ = cameraOrigin(camera.eyeZ);
 values.cameraPosition[0] = static_cast<float>(camera.eyeX - cameraOriginX);
 values.cameraPosition[1] = static_cast<float>(camera.eyeY - cameraOriginY);
 values.cameraPosition[2] = static_cast<float>(camera.eyeZ - cameraOriginZ);
 splitCameraPosition(camera.eyeX, camera.eyeY, camera.eyeZ, values.cameraPositionInt, values.cameraPositionFract);
 std::copy(std::begin(previousFrame.cameraPosition), std::end(previousFrame.cameraPosition), values.previousCameraPosition);
 std::copy(std::begin(previousFrame.cameraPositionFract), std::end(previousFrame.cameraPositionFract),
           values.previousCameraPositionFract);
 std::copy(std::begin(previousFrame.cameraPositionInt), std::end(previousFrame.cameraPositionInt),
           values.previousCameraPositionInt);
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
  render::directionToView(sun.directionX, sun.directionY, sun.directionZ, camera, values.sunPosition);
  render::directionToView(-sun.directionX, -sun.directionY, -sun.directionZ, camera, values.moonPosition);
  
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
  const float tickDelta = worldTime - std::floor(worldTime);
  const float celestialAngle = world->getTime(tickDelta);
  float sunAngle;
  if(celestialAngle < 0.75f) {
   sunAngle = celestialAngle + 0.25f;
  } else {
   sunAngle = celestialAngle - 0.75f;
  }
  values.sunAngle = sunAngle;
  values.shadowAngle = sunAngle < 0.5f ? sunAngle : sunAngle - 0.5f;
  
  // shadowLightPosition: moon when sun below horizon (sunAngle > 0.5), else sun.
  if(sunAngle > 0.5f) {
   std::copy(std::begin(values.moonPosition), std::end(values.moonPosition), values.shadowLightPosition);
  } else {
   std::copy(std::begin(values.sunPosition), std::end(values.sunPosition), values.shadowLightPosition);
  }
  
  values.rainStrength = world->getRainGradient(tickDelta);
  // Wetness EMA applied in ShaderPackManager::setFrameUniforms with pack halflife.
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
  }
  biomeData(world->getBiome(static_cast<int>(camera.eyeX), static_cast<int>(camera.eyeZ)), values);
  for(Entity* entity : world->globalEntities) {
   if(dynamic_cast<LightningEntity*>(entity) == nullptr) continue;
   values.lightningBoltPosition[0] = static_cast<float>(entity->x);
   values.lightningBoltPosition[1] = static_cast<float>(entity->y);
   values.lightningBoltPosition[2] = static_cast<float>(entity->z);
   values.lightningBoltPosition[3] = 1.0f;
   break;
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
   values.currentSelectedBlockId = render::resolveShaderObjectId(
       "block",
       blockId > 0 && blockId < Block::BLOCK_COUNT && Block::BLOCKS[static_cast<std::size_t>(blockId)] != nullptr
           ? Block::BLOCKS[static_cast<std::size_t>(blockId)]->getTranslationKey()
           : std::string{},
       blockId);
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
  values.currentPlayerArmor = std::clamp(static_cast<float>(player->inventory.getTotalArmorDurability()) / values.maxPlayerArmor, 0.0f, 1.0f);
  values.currentPlayerHunger = 1.0f;
  values.maxPlayerHunger = 20.0f;
  values.eyePosition[0] = static_cast<float>(player->x - cameraOriginX);
  values.eyePosition[1] = static_cast<float>(player->y + player->getEyeHeight() - cameraOriginY);
  values.eyePosition[2] = static_cast<float>(player->z - cameraOriginZ);
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
  for(int axis = 0; axis < 3; ++axis) values.relativeEyePosition[axis] = values.cameraPosition[axis] - values.eyePosition[axis];
  const Vec3d look = player->getLookVector();
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
       render::resolveShaderObjectId("entity", ::net::minecraft::entity::EntityRegistry::getId(*vehicle), 0);
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
   static float smoothSky = 0.0f;
   const float halfLifeSeconds = std::max(0.001f, eyeBrightnessHalflifeTicks / 20.0f);
   const float alpha = 1.0f - std::exp(-std::max(0.0f, values.frameTime) * 0.693147f / halfLifeSeconds);
   smoothBlock += (static_cast<float>(values.eyeBrightness[0]) - smoothBlock) * alpha;
   smoothSky += (static_cast<float>(values.eyeBrightness[1]) - smoothSky) * alpha;
   values.eyeBrightnessSmooth[0] = static_cast<int>(smoothBlock);
   values.eyeBrightnessSmooth[1] = static_cast<int>(smoothSky);
  }
 }
 {
  // Populated by ShaderPackManager::sampleCenterDepth (real center depthtex0 read).
  values.centerDepthSmooth = g_centerDepthSmooth;
 }
 if(firstFrame) {
  std::copy(std::begin(values.cameraPosition), std::end(values.cameraPosition), values.previousCameraPosition);
  std::copy(std::begin(values.cameraPositionFract), std::end(values.cameraPositionFract),
            values.previousCameraPositionFract);
  std::copy(std::begin(values.cameraPositionInt), std::end(values.cameraPositionInt),
            values.previousCameraPositionInt);
  std::copy(std::begin(values.gbufferProjection), std::end(values.gbufferProjection), values.gbufferPreviousProjection);
  std::copy(std::begin(values.gbufferModelView), std::end(values.gbufferModelView), values.gbufferPreviousModelView);
 }
 currentFrame = values;
 initialized = true;
 return values;
}
float updateCenterDepthSmooth(float windowDepth01, float nearPlane, float farPlane, float frameTime,
                              float halfLifeSeconds) {
 const float z = std::clamp(windowDepth01, 0.0f, 1.0f);
 const float n = std::max(nearPlane, 1e-4f);
 const float f = std::max(farPlane, n + 1e-3f);
 const float linear = (n * f) / std::max(1e-6f, f + z * (n - f));
 const float halfLife = std::max(0.001f, halfLifeSeconds);
 const float alpha = 1.0f - std::exp(-std::max(0.0f, frameTime) * 0.693147f / halfLife);
 g_centerDepthSmooth += (linear - g_centerDepthSmooth) * std::clamp(alpha, 0.0f, 1.0f);
 return g_centerDepthSmooth;
}
float updateWetnessSmooth(float rainStrength, float frameTime, float wetnessHalflife, float drynessHalflife) {
 const float target = std::clamp(rainStrength, 0.0f, 1.0f);
 const float halfLife = std::max(0.001f, target >= g_wetnessSmooth ? wetnessHalflife : drynessHalflife);
 const float alpha = 1.0f - std::exp(-std::max(0.0f, frameTime) * 0.693147f / halfLife);
 g_wetnessSmooth += (target - g_wetnessSmooth) * std::clamp(alpha, 0.0f, 1.0f);
 return g_wetnessSmooth;
}
} // namespace net::minecraft::client::render::shaderpack
