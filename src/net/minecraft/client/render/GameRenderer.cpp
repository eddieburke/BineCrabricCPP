#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/camera/GuiProjection.hpp"
#include <charconv>
#include <chrono>
#include <cmath>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/debug/VTuneTrace.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gui/screen/ChatScreen.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereContext.hpp"
#include "net/minecraft/client/render/atmosphere/SkyDome.hpp"
#include "net/minecraft/client/render/culling/Frustum.hpp"
#include "net/minecraft/client/render/chunk/TerrainLayers.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/uniforms/FrameData.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/util/UiScale.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/mod/model/ModModels.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/util/hit/HitResult.hpp"
#include "net/minecraft/util/math/Intersect.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#include <algorithm>
#include "net/minecraft/client/platform/glfw/Window.hpp"
#include <optional>
#include <thread>
#include <vector>
namespace net::minecraft::client::render {
namespace option = net::minecraft::client::option;
namespace math = net::minecraft::util::math;
namespace material = ::net::minecraft::block::material;
namespace mod = ::net::minecraft::mod;
namespace light = ::net::minecraft::world::light;
namespace {
constexpr int kBedBlockId = 26;
constexpr float kPiF = 3.14159265f;
constexpr float kHandDepth = 0.125f;
void updateSunLight(World* world, float tickDelta, ::net::minecraft::entity::Entity* camera) {
 if(world == nullptr || camera == nullptr) {
  return;
 }
 const float celestialAngle = world->getTime(tickDelta);
 const net::minecraft::client::Minecraft* celestialClient = net::minecraft::client::Minecraft::INSTANCE;
 const float sunPathRotation = celestialClient != nullptr && celestialClient->gameRenderer != nullptr
                                   ? celestialClient->gameRenderer->packDefinition().sunPathRotation
                                   : 25.0f;
 CelestialState state = makeCelestialState(celestialAngle, sunPathRotation);
 mod::CelestialStateEvent celestialEvent{world,
                                         camera,
                                         tickDelta,
                                         state.celestialAngle,
                                         state.sunAngle,
                                         state.shadowAngle,
                                         static_cast<int>((world->getTime() / 24000ULL) % 8ULL),
                                         state.sunDirectionWorld[0],
                                         state.sunDirectionWorld[1],
                                         state.sunDirectionWorld[2],
                                         state.moonDirectionWorld[0],
                                         state.moonDirectionWorld[1],
                                         state.moonDirectionWorld[2],
                                         state.day};
 mod::runtime::luaHookCelestialState(celestialEvent);
 const auto applyDirection = [](float x, float y, float z, float out[3]) {
  const float length = std::sqrt(x * x + y * y + z * z);
  if(std::isfinite(length) && length > 1.0e-6f) {
   out[0] = x / length;
   out[1] = y / length;
   out[2] = z / length;
  }
 };
 if(celestialEvent.overrideDirections) {
  state.celestialAngle = normalizeCelestialAngle(celestialEvent.celestialAngle, state.celestialAngle);
  state.sunAngle = normalizeCelestialAngle(celestialEvent.sunAngle, state.sunAngle);
  state.shadowAngle = normalizeCelestialAngle(celestialEvent.shadowAngle, state.shadowAngle);
  applyDirection(celestialEvent.sunX, celestialEvent.sunY, celestialEvent.sunZ, state.sunDirectionWorld);
  applyDirection(celestialEvent.moonX, celestialEvent.moonY, celestialEvent.moonZ, state.moonDirectionWorld);
  state.day = celestialEvent.day;
  for(int i = 0; i < 3; ++i) {
   state.shadowLightDirectionWorld[i] =
       state.day ? state.sunDirectionWorld[i] : state.moonDirectionWorld[i];
  }
  state.directionOverride = true;
 }
 state.moonPhase = std::clamp(celestialEvent.moonPhase, 0, 7);
 VT_TRACE_COUNTER("CelestialDirectionOverride", state.directionOverride ? 1 : 0);
 VT_TRACE_COUNTER("CelestialSunAngle", state.sunAngle);
 VT_TRACE_COUNTER("CelestialShadowAngle", state.shadowAngle);
 VT_TRACE_COUNTER("CelestialShadowLightY", state.shadowLightDirectionWorld[1]);
 core::setCelestialState(state);
 const float sunX = state.sunDirectionWorld[0];
 const float sunY = state.sunDirectionWorld[1];
 const float sunZ = state.sunDirectionWorld[2];
 const float daylight = std::clamp((sunY + 0.08f) / 0.28f, 0.0f, 1.0f);
 const float horizon = 1.0f - std::clamp(std::abs(sunY) * 5.0f, 0.0f, 1.0f);
 ::net::minecraft::world::light::SunLight sun;
 sun.directionX = sunX;
 sun.directionY = sunY;
 sun.directionZ = sunZ;
 sun.red = 1.0f;
 sun.green = 0.96f - horizon * 0.25f;
 sun.blue = 0.88f - horizon * 0.48f;
 sun.intensity = daylight;
 world->lightRegistry().setSun(sun);
 core::SkyUniforms skyUniforms = core::skyUniforms();
 skyUniforms.sunDirection[0] = sunX;
 skyUniforms.sunDirection[1] = sunY;
 skyUniforms.sunDirection[2] = sunZ;
 skyUniforms.sunIntensity = daylight;
 core::setSkyUniforms(skyUniforms);
}
[[nodiscard]] double vec3Distance(const Vec3d& a, const Vec3d& b) {
 const double dx = a.x - b.x;
 const double dy = a.y - b.y;
 const double dz = a.z - b.z;
 return std::sqrt(dx * dx + dy * dy + dz * dz);
}
[[nodiscard]] bool boxContains(const Box& box, const Vec3d& point) {
 return point.x >= box.minX && point.x <= box.maxX && point.y >= box.minY && point.y <= box.maxY &&
        point.z >= box.minZ && point.z <= box.maxZ;
}
[[nodiscard]] std::optional<HitResult> boxRaycast(const Box& box, const Vec3d& start, const Vec3d& end) {
 const double origin[3] = {start.x, start.y, start.z};
 const double dir[3] = {end.x - start.x, end.y - start.y, end.z - start.z};
 const double min[3] = {box.minX, box.minY, box.minZ};
 const double max[3] = {box.maxX, box.maxY, box.maxZ};
 const double t = math::raySlabIntersect(min, max, origin, dir, 1.0);
 if(t < 0.0) {
  return std::nullopt;
 }
 return HitResult{MathHelper::floor(start.x + dir[0] * t),
                  MathHelper::floor(start.y + dir[1] * t),
                  MathHelper::floor(start.z + dir[2] * t),
                  0,
                  {start.x + dir[0] * t, start.y + dir[1] * t, start.z + dir[2] * t}};
}
[[nodiscard]] float worldBrightness(World* world, int x, int y, int z) {
 if(world == nullptr) {
  return 1.0f;
 }
 return world->getLightBrightness(x, y, z);
}
[[nodiscard]] std::optional<HitResult> entityRaycast(World* world,
                                                     LivingEntity* camera,
                                                     double reach,
                                                     float tickDelta) {
 if(camera == nullptr || world == nullptr) {
  return std::nullopt;
 }
 const Vec3d start = camera->getPosition(tickDelta);
 const Vec3d look = camera->getLookVector(tickDelta);
 return world->raycast(start, start + Vec3d{look.x * reach, look.y * reach, look.z * reach});
}
[[nodiscard]] std::optional<HitResult> worldRaycast(World* world, const Vec3d& start, const Vec3d& end) {
 if(world == nullptr) {
  return std::nullopt;
 }
 return world->raycast(start, end);
}
[[nodiscard]] std::int64_t nowMillis() {
 return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
     .count();
}
} // namespace
GameRenderer::GameRenderer(Minecraft* clientIn)
    : client(clientIn),
      heldItemRenderer(std::make_unique<item::HeldItemRenderer>(clientIn)),
      lastInactiveTime(nowMillis()),
      shaderPipeline_(clientIn != nullptr ? std::make_unique<Pipeline>(
                                                Minecraft::getRunDirectory(), &clientIn->options,
                                                Minecraft::getRunDirectory() / "shader-cache")
                                          : nullptr) {}
GameRenderer::~GameRenderer() {
 shadowState_.targets.destroy();
}
const PackDefinition& GameRenderer::packDefinition() const noexcept {
 return shaderPipeline_ != nullptr ? shaderPipeline_->activeDefinition() : vanillaPackDefinition();
}
const PackDefinition& GameRenderer::meshDefinition() const noexcept {
 return shaderPipeline_ != nullptr ? shaderPipeline_->meshDefinition() : vanillaPackDefinition();
}
PackUniformValues GameRenderer::buildFrameUniforms(float tickDelta) const {
 const int width = client != nullptr ? std::max(1, client->displayWidth) : 1;
 const int height = client != nullptr ? std::max(1, client->displayHeight) : 1;
 const float worldTime = static_cast<float>(ticks) + tickDelta;
 const PackDefinition& definition = packDefinition();
 const float eyeHalf = definition.eyeBrightnessHalflife;
 const FrameRenderCamera shadowCamera =
     shadowmap::makeShadowCamera(definition, frameCamera_, core::celestialState());
 const bool shadowAvailable = definition.shadowMapResolution > 0 && definition.shadowDistance > 0.0f &&
                              client != nullptr && client->world != nullptr;
 const int shadowResolution =
     shadowAvailable ? std::clamp(definition.shadowMapResolution, 256, 16384) : 0;
 return buildShaderFrameData(width, height, worldTime,
                             shadowResolution, shaderPipeline_->sceneColorCount() > 1,
                             shadowAvailable, frameCamera_, shadowCamera,
                             client != nullptr ? client->world : nullptr,
                             eyeHalf);
}
void GameRenderer::updateCamera() {
 if(client == nullptr) {
  return;
 }
 lastViewBob = viewBob;
 prevThirdPersonDistance = thirdPersonDistance;
 prevThirdPersonYaw = thirdPersonYaw;
 prevThirdPersonPitch = thirdPersonPitch;
 prevCameraRoll = cameraRoll;
 prevCameraRollAmount = cameraRollAmount;
 if(client->camera == nullptr) {
  client->camera = client->player;
 }
 float ambient = 1.0f;
 if(client->world != nullptr && client->camera != nullptr) {
  ambient = worldBrightness(client->world,
                            MathHelper::floor(client->camera->x),
                            MathHelper::floor(client->camera->y),
                            MathHelper::floor(client->camera->z));
 }
 const float viewDistBlend = static_cast<float>(3 - client->options.viewDistance) / 3.0f;
 const float blended = ambient * (1.0f - viewDistBlend) + viewDistBlend;
 viewBob += (blended - viewBob) * 0.1f;
 ++ticks;
 if(heldItemRenderer != nullptr) {
  heldItemRenderer->tick();
 }
 renderRain();
}
void GameRenderer::updateTargetedEntity(float tickDelta) {
 if(client == nullptr || client->camera == nullptr || client->world == nullptr ||
    client->interactionManager == nullptr) {
  return;
 }
 auto* livingCamera = dynamic_cast<LivingEntity*>(client->camera);
 if(livingCamera == nullptr) {
  return;
 }
 double reach = static_cast<double>(client->interactionManager->getReachDistance());
 client->crosshairTarget = entityRaycast(client->world, livingCamera, reach, tickDelta);
 double reachAlongLook = reach;
 const Vec3d eyePos = livingCamera->getPosition(tickDelta);
 if(client->crosshairTarget.has_value()) {
  reachAlongLook = vec3Distance(client->crosshairTarget->pos, eyePos);
 }
 if(reachAlongLook > 3.0) {
  reachAlongLook = 3.0;
 }
 reach = reachAlongLook;
 const Vec3d look = livingCamera->getLookVector(tickDelta);
 const Vec3d end = eyePos + look * reach;
 targetedEntity = nullptr;
 double closest = 0.0;
 const Box queryBox =
     client->camera->boundingBox.stretch(look.x * reach, look.y * reach, look.z * reach).expand(1.0);
 const std::vector<Entity*> entities = client->world->getEntities(client->camera, queryBox);
 for(Entity* entity : entities) {
  if(entity == nullptr || !entity->isCollidable()) {
   continue;
  }
  const float margin = entity->getTargetingMargin();
  const Box hitBox = entity->boundingBox.expand(margin);
  const std::optional<HitResult> hit = boxRaycast(hitBox, eyePos, end);
  if(boxContains(hitBox, eyePos)) {
   if(!(0.0 < closest) && closest != 0.0) {
    continue;
   }
   targetedEntity = entity;
   closest = 0.0;
   continue;
  }
  if(!hit.has_value()) {
   continue;
  }
  const double dist = vec3Distance(eyePos, hit->pos);
  if(!(dist < closest) && closest != 0.0) {
   continue;
  }
  targetedEntity = entity;
  closest = dist;
 }
 mod::model::ModelRaycastHit modelHit;
 if(mod::model::raycastModelInstances(
        eyePos.x, eyePos.y, eyePos.z, look.x, look.y, look.z, reach, modelHit)) {
  const double limit = targetedEntity != nullptr ? (closest == 0.0 ? 0.0 : closest) : reach;
  if(modelHit.distance < limit) {
   const std::size_t colon = modelHit.tag.find(':');
   auto* clientWorld = dynamic_cast<ClientWorld*>(client->world);
   if(colon != std::string::npos && clientWorld != nullptr) {
    const std::string idText = modelHit.tag.substr(colon + 1);
    int entityId = 0;
    const auto* end = idText.data() + idText.size();
    const auto parsed = std::from_chars(idText.data(), end, entityId);
    if(parsed.ec == std::errc() && parsed.ptr == end) {
     if(auto* e = clientWorld->getEntity(entityId)) {
      targetedEntity = e;
      closest = modelHit.distance;
     }
    }
   }
  }
 }
 if(targetedEntity != nullptr) {
  client->crosshairTarget = HitResult(targetedEntity, targetedEntity->position());
 }
}
float GameRenderer::getFov(float tickDelta) const {
 if(client == nullptr || client->camera == nullptr) {
  return 70.0f;
 }
 auto* living = dynamic_cast<LivingEntity*>(client->camera);
 if(living == nullptr) {
  return 70.0f;
 }
 float fov = 70.0f;
 if(living->isInFluid(material::Material::WATER)) {
  fov = 60.0f;
 }
 if(living->health <= 0) {
  const float death = static_cast<float>(living->deathTime) + tickDelta;
  fov /= (1.0f - 500.0f / (death + 500.0f)) * 2.0f + 1.0f;
 }
 fov = option::adjustFieldOfView(fov, frameSettings_);
 mod::FovEvent event{living, tickDelta, fov};
 mod::runtime::luaHookFov(event);
 return event.fov + prevCameraRoll + (cameraRoll - prevCameraRoll) * tickDelta;
}
void GameRenderer::applyDamageTiltEffect(float tickDelta, math::Matrix4f& modelView) {
 if(client == nullptr || client->camera == nullptr) {
  return;
 }
 const auto* living = dynamic_cast<const LivingEntity*>(client->camera);
 if(living == nullptr) {
  return;
 }
 float hurt = static_cast<float>(living->hurtTime) - tickDelta;
 if(living->health <= 0) {
  const float death = static_cast<float>(living->deathTime) + tickDelta;
  modelView.rotate(40.0f - 8000.0f / (death + 200.0f), 0.0f, 0.0f, 1.0f);
 }
 if(hurt < 0.0f) {
  return;
 }
 hurt /= static_cast<float>(living->damagedTime);
 hurt = MathHelper::sin(hurt * hurt * hurt * hurt * kPiF);
 const float swing = living->damagedSwingDir;
 modelView.rotate(-swing, 0.0f, 1.0f, 0.0f);
 modelView.rotate(-hurt * 14.0f, 0.0f, 0.0f, 1.0f);
 modelView.rotate(swing, 0.0f, 1.0f, 0.0f);
}
void GameRenderer::applyViewBobbing(float tickDelta, math::Matrix4f& modelView) {
 if(client == nullptr) {
  return;
 }
 auto* player = dynamic_cast<PlayerEntity*>(client->camera);
 if(player == nullptr) {
  return;
 }
 const float speedDelta = player->horizontalSpeed - player->prevHorizontalSpeed;
 const float phase = -(player->horizontalSpeed + speedDelta * tickDelta);
 const float stepBob =
     player->prevStepBobbingAmount + (player->stepBobbingAmount - player->prevStepBobbingAmount) * tickDelta;
 const float tiltBob = player->prevTilt + (player->tilt - player->prevTilt) * tickDelta;
 modelView.translate(
     MathHelper::sin(phase * kPiF) * stepBob * 0.5f, -std::abs(MathHelper::cos(phase * kPiF) * stepBob), 0.0f);
 modelView.rotate(MathHelper::sin(phase * kPiF) * stepBob * 3.0f, 0.0f, 0.0f, 1.0f);
 modelView.rotate(std::abs(MathHelper::cos(phase * kPiF - 0.2f) * stepBob) * 5.0f, 1.0f, 0.0f, 0.0f);
 modelView.rotate(tiltBob, 1.0f, 0.0f, 0.0f);
}
void GameRenderer::applyCameraTransform(float tickDelta, math::Matrix4f& modelView) {
 if(client == nullptr || client->camera == nullptr) {
  return;
 }
 if(frameCamera_.hasExplicitModelView) {
  modelView.set(frameCamera_.explicitModelView);
  return;
 }
 auto* living = dynamic_cast<LivingEntity*>(client->camera);
 if(living == nullptr) {
  return;
 }
 if(frameCamera_.customView) {
  modelView.rotate(frameCamera_.roll, 0.0f, 0.0f, 1.0f);
  modelView.rotate(frameCamera_.pitch, 1.0f, 0.0f, 0.0f);
  modelView.rotate(frameCamera_.yaw + 180.0f, 0.0f, 1.0f, 0.0f);
  return;
 }
 float eyeOffset = living->standingEyeHeight - 1.62f;
 double interpX = living->lastTickX + (living->x - living->lastTickX) * static_cast<double>(tickDelta);
 double interpY = living->lastTickY + (living->y - living->lastTickY) * static_cast<double>(tickDelta) -
                  static_cast<double>(eyeOffset);
 double interpZ = living->lastTickZ + (living->z - living->lastTickZ) * static_cast<double>(tickDelta);
 modelView.rotate(
     prevCameraRollAmount + (cameraRollAmount - prevCameraRollAmount) * tickDelta, 0.0f, 0.0f, 1.0f);
 if(living->isSleeping()) {
  eyeOffset += 1.0f;
  modelView.translate(0.0f, 0.3f, 0.0f);
  if(!client->options.debugCamera && client->world != nullptr) {
   const int blockId = client->world->getBlockId(
       MathHelper::floor(living->x), MathHelper::floor(living->y), MathHelper::floor(living->z));
   if(blockId == kBedBlockId) {
    const int meta = client->world->getBlockMeta(
        MathHelper::floor(living->x), MathHelper::floor(living->y), MathHelper::floor(living->z));
    const int facing = static_cast<int>(meta) & 3;
    modelView.rotate(static_cast<float>(facing) * 90.0f, 0.0f, 1.0f, 0.0f);
   }
   modelView.rotate(
       living->prevYaw + (living->yaw - living->prevYaw) * tickDelta + 180.0f, 0.0f, -1.0f, 0.0f);
   modelView.rotate(
       living->prevPitch + (living->pitch - living->prevPitch) * tickDelta, -1.0f, 0.0f, 0.0f);
  }
 } else if(client->options.thirdPerson) {
  double camDist =
      prevThirdPersonDistance + (thirdPersonDistance - prevThirdPersonDistance) * static_cast<double>(tickDelta);
  if(client->options.debugCamera) {
   const float dbgYaw = prevThirdPersonYaw + (thirdPersonYaw - prevThirdPersonYaw) * tickDelta;
   const float dbgPitch = prevThirdPersonPitch + (thirdPersonPitch - prevThirdPersonPitch) * tickDelta;
   modelView.translate(0.0f, 0.0f, static_cast<float>(-camDist));
   modelView.rotate(dbgPitch, 1.0f, 0.0f, 0.0f);
   modelView.rotate(dbgYaw, 0.0f, 1.0f, 0.0f);
  } else if(client->world != nullptr) {
   const float baseYaw = living->yaw;
   const float basePitch = living->pitch;
   double offsetX = static_cast<double>(-MathHelper::sin(baseYaw / 180.0f * kPiF) *
                                        MathHelper::cos(basePitch / 180.0f * kPiF)) *
                    camDist;
   double offsetZ = static_cast<double>(MathHelper::cos(baseYaw / 180.0f * kPiF) *
                                        MathHelper::cos(basePitch / 180.0f * kPiF)) *
                    camDist;
   double offsetY = static_cast<double>(-MathHelper::sin(basePitch / 180.0f * kPiF)) * camDist;
   for(int corner = 0; corner < 8; ++corner) {
    float sx = static_cast<float>((corner & 1) * 2 - 1);
    float sy = static_cast<float>((corner >> 1 & 1) * 2 - 1);
    float sz = static_cast<float>((corner >> 2 & 1) * 2 - 1);
    sx *= 0.1f;
    sy *= 0.1f;
    sz *= 0.1f;
    const Vec3d rayStart{interpX + static_cast<double>(sx),
                         interpY + static_cast<double>(sy),
                         interpZ + static_cast<double>(sz)};
    const Vec3d rayEnd{interpX - offsetX + static_cast<double>(sx) + static_cast<double>(sz),
                       interpY - offsetY + static_cast<double>(sy),
                       interpZ - offsetZ + static_cast<double>(sz)};
    const std::optional<HitResult> hit = worldRaycast(client->world, rayStart, rayEnd);
    if(!hit.has_value()) {
     continue;
    }
    const double dist = vec3Distance(hit->pos, Vec3d{interpX, interpY, interpZ});
    if(dist < camDist) {
     camDist = dist;
    }
   }
   modelView.rotate(living->pitch - basePitch, 1.0f, 0.0f, 0.0f);
   modelView.rotate(living->yaw - baseYaw, 0.0f, 1.0f, 0.0f);
   modelView.translate(0.0f, 0.0f, static_cast<float>(-camDist));
   modelView.rotate(baseYaw - living->yaw, 0.0f, 1.0f, 0.0f);
   modelView.rotate(basePitch - living->pitch, 1.0f, 0.0f, 0.0f);
  }
 } else {
  modelView.translate(0.0f, 0.0f, -0.1f);
 }
 if(!client->options.debugCamera) {
  modelView.rotate(living->prevPitch + (living->pitch - living->prevPitch) * tickDelta, 1.0f, 0.0f, 0.0f);
  modelView.rotate(living->prevYaw + (living->yaw - living->prevYaw) * tickDelta + 180.0f, 0.0f, 1.0f, 0.0f);
 }
 modelView.translate(0.0f, eyeOffset, 0.0f);
}
void GameRenderer::renderWorld(float tickDelta,
                               float fov,
                               math::Matrix4f& modelView,
                               math::Matrix4f& projection) {
 if(client == nullptr) {
  return;
 }
 int viewport[4]{0, 0, client->displayWidth, client->displayHeight};
 if(!core::getCachedViewport(viewport)) {
  core::getIntegerv(gl::query::Viewport, viewport);
 }
 const float aspect = viewport[3] != 0 ? static_cast<float>(viewport[2]) / static_cast<float>(viewport[3]) : 1.0f;
 projection.identity();
 if(!frameCamera_.customView && !frameCamera_.shadowPass) {
  frameCamera_.nearPlane = frameSettings_.renderDistance.nearPlane();
  frameCamera_.farPlane = frameSettings_.renderDistance.farPlane();
  frameCamera_.renderDistanceBlocks = frameSettings_.renderDistance.sectionCoverageBlocks();
 }
 if(!frameCamera_.orthographic) {
  const float focal = 1.0f / std::tan(fov * 3.14159265f / 360.0f);
  frameCamera_.projectionX = focal / (aspect != 0.0f ? aspect : 1.0f);
  frameCamera_.projectionY = focal;
 }
 frameCamera_.zoomScale = static_cast<float>(zoom);
 frameCamera_.zoomOffsetX = static_cast<float>(zoomX);
 frameCamera_.zoomOffsetY = static_cast<float>(-zoomY);
 {
  math::Matrix4f proj;
  buildCameraProjection(proj.data(), frameCamera_);
  projection = proj;
 }
 modelView.identity();
 frameCamera_.hasBobModelView = false;
 if(!frameCamera_.customView) {
  applyDamageTiltEffect(tickDelta, modelView);
  if(client->options.bobView) {
   applyViewBobbing(tickDelta, modelView);
  }
  if(client->player != nullptr) {
   const float distortion =
       client->player->lastScreenDistortion +
       (client->player->screenDistortion - client->player->lastScreenDistortion) * tickDelta;
   if(distortion > 0.0f) {
    float scale = 5.0f / (distortion * distortion + 5.0f) - distortion * 0.04f;
    scale *= scale;
    modelView.rotate((static_cast<float>(ticks) + tickDelta) * 20.0f, 0.0f, 1.0f, 1.0f);
    modelView.scale(1.0f / scale, 1.0f, 1.0f);
    modelView.rotate(-((static_cast<float>(ticks) + tickDelta) * 20.0f), 0.0f, 1.0f, 1.0f);
   }
  }
  const math::Matrix4f& bobStack = modelView;
  const math::Matrix4f identity = math::Matrix4f::identityMatrix();
  if(std::memcmp(bobStack.m, identity.m, sizeof(identity.m)) != 0) {
   std::memcpy(frameCamera_.bobModelView, bobStack.m, sizeof(frameCamera_.bobModelView));
   frameCamera_.hasBobModelView = true;
  }
 }
 applyCameraTransform(tickDelta, modelView);
}
void GameRenderer::renderFirstPersonHand(float tickDelta) {
 if(client == nullptr || heldItemRenderer == nullptr || frameCamera_.hideFirstPersonHand) {
  return;
 }
 int viewport[4]{0, 0, client->displayWidth, client->displayHeight};
 if(!core::getCachedViewport(viewport)) {
  core::getIntegerv(gl::query::Viewport, viewport);
 }
 const float aspect = viewport[3] != 0 ? static_cast<float>(viewport[2]) / static_cast<float>(viewport[3]) : 1.0f;
 FrameRenderCamera handCamera = frameCamera_;
 handCamera.orthographic = false;
 {
  const float focal = 1.0f / std::tan(getFov(tickDelta) * 3.14159265f / 360.0f);
  handCamera.projectionX = focal / (aspect != 0.0f ? aspect : 1.0f);
  handCamera.projectionY = focal;
 }
 handCamera.nearPlane = frameSettings_.renderDistance.nearPlane();
 handCamera.farPlane = frameSettings_.renderDistance.farPlane();
 handCamera.renderDistanceBlocks = frameSettings_.renderDistance.sectionCoverageBlocks();
 handCamera.depthScale = kHandDepth;
 math::Matrix4f handProjection;
 buildCameraProjection(handProjection.data(), handCamera);
 const core::ScopedDrawCameraState handCameraGuard;
 math::Matrix4f identity;
 math::Matrix4f identityInverse = identity;
 math::Matrix4f projectionInverse = handProjection;
 projectionInverse.invert();
 const float zero[3] = {0.0f, 0.0f, 0.0f};
 core::setDrawCameraState(identity.m, handProjection.m, identityInverse.m, projectionInverse.m, zero);
 const core::DepthScope handDepth(true, true);
 auto* living = dynamic_cast<LivingEntity*>(client->camera);
 entity::EntityRenderDispatcher::instance().setCameraEntity(living);
 {
  math::Matrix4f modelView;
  applyDamageTiltEffect(tickDelta, modelView);
  if(client->options.bobView) {
   applyViewBobbing(tickDelta, modelView);
  }
  core::setPassModelView(modelView);
  if(living != nullptr) {
   mod::FirstPersonHandRenderEvent event{living, tickDelta, 0, false};
   mod::runtime::luaHookFirstPersonHand(event);
   if(event.canceled) {
    return;
   }
  }
  if(living != nullptr && !client->options.thirdPerson && !living->isSleeping() && !client->options.hideHud) {
   if(client->world != nullptr) {
    heldItemRenderer->render(tickDelta);
   }
  }
 }
 if(living != nullptr && !client->options.thirdPerson && !living->isSleeping()) {
  heldItemRenderer->renderScreenOverlays(tickDelta);
 }
}
void GameRenderer::onFrameUpdate(float tickDelta) {
 if(client == nullptr) {
  return;
 }
 frameSettings_ = option::renderSettings(client->options, meshDefinition());
 if(!platform::glfw::Window::isActive()) {
  if(nowMillis() - lastInactiveTime > 500) {
   client->pauseGame();
  }
 } else {
  lastInactiveTime = nowMillis();
 }
 if(client->focused && client->player != nullptr) {
  input::InputSystem::instance().pollMouseLook();
  const float sensitivity = client->options.mouseSensitivity * 0.6f + 0.2f;
  const float scale = sensitivity * sensitivity * sensitivity * 8.0f;
  float deltaYaw = static_cast<float>(input::InputSystem::instance().mouseLookDeltaX()) * scale;
  float deltaPitch = static_cast<float>(input::InputSystem::instance().mouseLookDeltaY()) * scale;
  int invert = 1;
  if(client->options.invertYMouse) {
   invert = -1;
  }
  if(client->options.cinematicMode) {
   deltaYaw = cinematicCameraYawSmoother.smooth(deltaYaw, 0.05f * scale);
   deltaPitch = cinematicCameraPitchSmoother.smooth(deltaPitch, 0.05f * scale);
  } else if(frameSettings_.smoothInput) {
   deltaYaw = yawSmoother.smooth(deltaYaw, 0.15f * scale);
   deltaPitch = pitchSmoother.smooth(deltaPitch, 0.15f * scale);
  }
  client->player->changeLookDirection(deltaYaw, deltaPitch * static_cast<float>(invert));
 }
 if(client->skipGameRender) {
  return;
 }
 shaderPipeline_->poll();
 if(client->world != nullptr) {
  renderFrame(tickDelta);
 }
 {
  if(client->world != nullptr) {
   if(!client->options.hideHud || client->currentScreen() != nullptr) {
    shaderPipeline_->setPipelinePhase(WorldPipelinePhase::None);
    const bool chatOpen = dynamic_cast<gui::screen::ChatScreen*>(client->currentScreen()) != nullptr;
    {
     const int width = std::max(1, client->displayWidth);
     const int height = std::max(1, client->displayHeight);
     gui_proj::begin(util::uiScale(client->options, width, height), width, height, false);
    }
    client->inGameHud.render(tickDelta, chatOpen, 0, 0);
   }
  } else {
   core::viewport(0, 0, client->displayWidth, client->displayHeight);
   core::clear(gl::attrib::ColorBufferBit | gl::attrib::DepthBufferBit);
   {
    const int width = std::max(1, client->displayWidth);
    const int height = std::max(1, client->displayHeight);
    gui_proj::begin(util::uiScale(client->options, width, height), width, height, false);
   }
  }
  if(client->currentScreen() != nullptr) {
   shaderPipeline_->setPipelinePhase(WorldPipelinePhase::None);
   input::InputSystem& input = input::InputSystem::instance();
   const util::UiScale scale = util::uiScale(client->options, client->displayWidth, client->displayHeight);
   const auto [mouseX, mouseY] = util::mapScreenMouse(client->displayWidth,
                                                      client->displayHeight,
                                                      scale.scaledWidth,
                                                      scale.scaledHeight,
                                                      input.mouseX(),
                                                      input.mouseY());
   gui_proj::begin(scale, client->displayWidth, client->displayHeight, true);
   client->currentScreen()->render(mouseX, mouseY, tickDelta);
  }
 }
}
bool GameRenderer::beginSceneCapture() {
 if(client == nullptr || shaderPipeline_ == nullptr) {
  return false;
 }
 const int width = std::max(1, client->displayWidth);
 const int height = std::max(1, client->displayHeight);
 if(!shaderPipeline_->ensureSceneTargets(width, height)) {
  return false;
 }
 shaderPipeline_->clearScene(core::fog().color[0], core::fog().color[1], core::fog().color[2]);
 core::clear(gl::attrib::DepthBufferBit);
 return true;
}
void GameRenderer::resolveSceneCapture() {
 if(client == nullptr || shaderPipeline_ == nullptr) {
  return;
 }
 shaderPipeline_->endScene();
 const int width = std::max(1, client->displayWidth);
 const int height = std::max(1, client->displayHeight);
 gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer), 0);
 core::viewport(0, 0, width, height);
 core::clear(gl::attrib::ColorBufferBit | gl::attrib::DepthBufferBit);
 shaderPipeline_->renderPostProcess(frameShadow_.depthTexture, frameShadow_.opaqueDepthTexture,
                                    frameShadow_.colorTextures.data(), frameShadow_.colorCount,
                                    frameShadow_.colorAltTextures.data());
 shaderPipeline_->reset();
 core::viewport(0, 0, width, height);
}
namespace {
using atmosphere::AtmosphereContext;
[[nodiscard]] ItemStack selectedItemOrEmpty(PlayerEntity* player) {
 if(player == nullptr) {
  return {};
 }
 const ItemStack* stack = player->inventory.getSelectedItem();
 return stack != nullptr ? *stack : ItemStack{};
}
[[nodiscard]] AtmosphereContext makeAtmosphereContext(Minecraft* client,
                                                      const option::RenderSettings& settings,
                                                      int atmosphereTicks) {
 return AtmosphereContext{
     .client = client,
     .world = client->world,
     .textureManager = &client->textureManager,
     .camera = client->camera,
     .settings = settings,
     .atmosphereTicks = atmosphereTicks,
 };
}
void applyEntityLightingRig(const FrameRenderCamera& camera, core::WorldLightUniforms rig) {
 constexpr float kLen = 1.2369317f;
 constexpr float kKeyX = 0.2f / kLen;
 constexpr float kKeyY = 1.0f / kLen;
 constexpr float kKeyZ = -0.7f / kLen;
 directionToView(kKeyX, kKeyY, kKeyZ, camera, rig.sunDirView);
 directionToView(-kKeyX, kKeyY, -kKeyZ, camera, rig.fillDirView);
 rig.sunColor[0] = rig.sunColor[1] = rig.sunColor[2] = 1.0f;
 rig.sunIntensity = 0.6f;
 rig.fillIntensity = 0.6f;
 rig.ambient[0] = rig.ambient[1] = rig.ambient[2] = 0.4f;
 core::setWorldLight(rig);
}
void bindTerrainTexture(int terrainTextureId) {
 if(terrainTextureId < 0) {
  return;
 }
 core::activeTexture(gl::tex::Texture0);
 core::bindTexture(terrainTextureId);
}
void drawSolidTerrain(WorldRenderer& worldRenderer,
                      LivingEntity& camera,
                      int terrainTextureId) {
 bindTerrainTexture(terrainTextureId);
 const RenderPassScope solidScope(RenderType::solid());
 core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
 worldRenderer.render(camera, chunk::terrain_layer::Solid);
}
void drawCutoutTerrain(WorldRenderer& worldRenderer,
                       LivingEntity& camera,
                       int terrainTextureId) {
 bindTerrainTexture(terrainTextureId);
 {
  const RenderPassScope cutoutScope(RenderType::cutout());
  core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
  worldRenderer.render(camera, chunk::terrain_layer::Cutout);
 }
 bindTerrainTexture(terrainTextureId);
 const RenderPassScope interiorScope(RenderType::cutoutInterior());
 core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
 worldRenderer.render(camera, chunk::terrain_layer::CutoutInterior);
}
void drawTranslucentTerrain(WorldRenderer& worldRenderer,
                            LivingEntity& camera,
                            float tickDelta,
                            int terrainTextureId,
                            bool fancyGraphics) {
 bindTerrainTexture(terrainTextureId);
 {
  const RenderPassScope translucentScope(RenderType::translucent());
  if(fancyGraphics) {
   {
    const core::ColorMaskScope colorMaskPass(false, false, false, false);
    worldRenderer.render(camera, chunk::terrain_layer::Translucent, false);
   }
   worldRenderer.renderLastChunks(chunk::terrain_layer::Translucent, static_cast<double>(tickDelta));
  } else {
   worldRenderer.render(camera, chunk::terrain_layer::Translucent);
  }
 }
 core::depthMask(true);
 core::cullBackFaces();
}
void renderBlockOverlay(WorldRenderer& worldRenderer,
                        Minecraft* client,
                        PlayerEntity& player,
                        float tickDelta) {
 if(client == nullptr || !client->crosshairTarget.has_value()) {
  return;
 }
 const ItemStack hand = selectedItemOrEmpty(&player);
 worldRenderer.renderMiningProgress(&player, *client->crosshairTarget, 0, hand, tickDelta);
 worldRenderer.renderBlockOutline(&player, *client->crosshairTarget, 0, hand, tickDelta);
}
template <typename Draw>
bool renderWorldStage(const AtmosphereContext& context,
                      float tickDelta,
                      mod::WorldRenderStage stage,
                      bool enabled,
                      bool shadowPass,
                      Draw&& draw) {
 (void)shadowPass;
 mod::WorldRenderEvent event{
     context.world,
     context.camera,
     tickDelta,
     stage,
     mod::RenderHookMoment::Before,
 };
 event.stageEnabled = enabled;
 event.renderDistance = context.settings.renderDistance.blocks;
 if(context.world != nullptr) {
  event.rainStrength = context.world->getRainGradient(tickDelta);
  event.starBrightness = context.world->calculateSkyLightIntensity(tickDelta) * (1.0f - event.rainStrength);
  const Vec3d cloudColor = context.world->getCloudColor(tickDelta);
  event.cloudRed = static_cast<float>(cloudColor.x);
  event.cloudGreen = static_cast<float>(cloudColor.y);
  event.cloudBlue = static_cast<float>(cloudColor.z);
  if(context.world->dimension != nullptr) {
   event.cloudBaseHeight = option::cloudHeightOffset(
       context.world->dimension->getCloudHeight(), context.settings);
  }
 }
 mod::runtime::luaHookWorldRender(event);
 if(enabled && !event.cancelVanilla) {
  if constexpr(requires { draw(event); }) {
   draw(event);
  } else {
   draw();
  }
  event.vanillaStageRan = true;
 }
 event.moment = mod::RenderHookMoment::After;
 mod::runtime::luaHookWorldRender(event);
 return event.vanillaStageRan;
}
} // namespace
void GameRenderer::renderFrame(float tickDelta) {
 if(client == nullptr) {
  return;
 }
 bool captured = false;
 {
  captured = beginSceneCapture();
 }
 const int width = std::max(1, client->displayWidth);
 const int height = std::max(1, client->displayHeight);
 renderToCurrentTarget(tickDelta, FrameRenderCamera{}, getFov(tickDelta), width, height, false, captured);
 if(captured) {
  resolveSceneCapture();
 }
}
bool GameRenderer::renderWorldToFbo(unsigned int fbo,
                                    int width,
                                    int height,
                                    float tickDelta,
                                    const FrameRenderCamera& camera,
                                    float fov) {
 if(client == nullptr || fbo == 0 || width <= 0 || height <= 0 || !std::isfinite(tickDelta) ||
    !std::isfinite(fov)) {
  return false;
 }
 int prevFbo = 0;
 int prevViewport[4] = {0, 0, 0, 0};
 core::getIntegerv(0x8CA6, &prevFbo);
 core::getIntegerv(0x0BA2, prevViewport);
 const core::BlendScope blendGuard(core::blendEnabled());
 const core::DepthScope depthGuard(core::depthTestEnabled(), core::depthWriteEnabled());
 const core::CullScope cullGuard(core::cullEnabled());
 const core::TextureBindScope textureGuard;
 const core::ScopedDrawCameraState drawCameraGuard;
 const FrameRenderCamera prevFrameCamera = frameCamera_;
 const FrameRenderCamera prevPublishedCamera = core::cameraFrame();
 WorldRenderer* worldRenderer = client->worldRenderer.get();
 Entity* prevWorldCam = nullptr;
 bool prevRenderCamEntity = false;
 std::optional<WorldRenderer::ScopedCullState> chunkCullGuard;
 if(worldRenderer != nullptr) {
  prevWorldCam = worldRenderer->cameraEntity();
  prevRenderCamEntity = worldRenderer->renderCameraEntity();
  chunkCullGuard.emplace(*worldRenderer);
 }
 {
  gl::GLCore::bindFramebuffer(gl::framebuffer::Framebuffer, fbo);
  VT_TRACE_COUNTER("FramebufferBinds", 1);
  core::viewport(0, 0, width, height);
 }
 const PackDefinition& packDef = packDefinition();
 const bool hwOffset = camera.shadowPass && packDef.shadowHardwareOffset;
 if(hwOffset) {
  float factor = packDef.shadowHardwareOffsetFactor;
  float units = packDef.shadowHardwareOffsetUnits;
  if(client != nullptr && client->options.shadowDisablePolyOffset) {
   factor = 0.0f;
   units = 0.0f;
  }
  core::enablePolygonOffset();
  core::polygonOffset(factor, units);
 }
 renderToCurrentTarget(std::clamp(tickDelta, 0.0f, 1.0f), camera, std::clamp(fov, 1.0f, 179.0f), width,
                       height, true, false);
 if(hwOffset) {
  core::disablePolygonOffset();
 }
 if(worldRenderer != nullptr) {
  worldRenderer->setCamera(prevWorldCam);
  worldRenderer->setRenderCameraEntity(prevRenderCamEntity);
  chunkCullGuard.reset();
 }
 frameCamera_ = prevFrameCamera;
 core::setCameraFrame(prevPublishedCamera);
 {
  gl::GLCore::bindFramebuffer(gl::framebuffer::Framebuffer, static_cast<unsigned>(prevFbo));
  VT_TRACE_COUNTER("FramebufferBinds", 1);
  core::viewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
 }
 return true;
}
void GameRenderer::renderToCurrentTarget(float tickDelta,
                                         const FrameRenderCamera& cameraFrame,
                                         float fov,
                                         int viewportWidth,
                                         int viewportHeight,
                                         bool renderCameraEntity,
                                         bool colorAlreadyCleared) {
 if(client == nullptr) {
  return;
 }
 const Pipeline::PhaseScope pipelinePhase(
     shaderPipeline_.get(),
     cameraFrame.shadowPass ? WorldPipelinePhase::Shadow
                            : WorldPipelinePhase::World);
 core::viewport(0, 0, viewportWidth, viewportHeight);
 if(client->camera == nullptr) {
  client->camera = client->player;
 }
 if(!cameraFrame.shadowPass) {
  updateTargetedEntity(tickDelta);
 }
 auto* camera = dynamic_cast<LivingEntity*>(client->camera);
 WorldRenderer* worldRenderer = client->worldRenderer.get();
 if(camera == nullptr || client->world == nullptr || worldRenderer == nullptr) {
  return;
 }
 core::cullBackFaces();
 core::depthTest();
 core::depthMask(true);
 math::Matrix4f modelView;
 math::Matrix4f projection;
 frameCamera_ = cameraFrame;
 worldRenderer->setCamera(camera);
 worldRenderer->setRenderCameraEntity(renderCameraEntity);
 if(!frameCamera_.customView) {
  const float rollAmount = prevCameraRollAmount + (cameraRollAmount - prevCameraRollAmount) * tickDelta;
  frameCamera_.x = camera->lastTickX + (camera->x - camera->lastTickX) * static_cast<double>(tickDelta);
  frameCamera_.y = camera->lastTickY + (camera->y - camera->lastTickY) * static_cast<double>(tickDelta);
  frameCamera_.z = camera->lastTickZ + (camera->z - camera->lastTickZ) * static_cast<double>(tickDelta);
  frameCamera_.yaw = camera->prevYaw + (camera->yaw - camera->prevYaw) * tickDelta;
  frameCamera_.pitch = camera->prevPitch + (camera->pitch - camera->prevPitch) * tickDelta;
  frameCamera_.roll = rollAmount;
  frameCamera_.customView = false;
  frameCamera_.hideFirstPersonHand = false;
  mod::CameraSetupEvent cameraEvent{};
  cameraEvent.tickDelta = tickDelta;
  cameraEvent.frame = &frameCamera_;
  mod::runtime::luaHookCameraSetup(cameraEvent);
 }
 if(!renderCameraEntity) {
  client->world->setChunkCacheCenterFromBlockPos(MathHelper::floor(frameCamera_.x),
                                                 MathHelper::floor(frameCamera_.z));
 }
 const bool fancyGraphics = client->options.fancyGraphics;
 const int terrainTextureId = client->textureManager.getTextureId("/terrain.png");
 const AtmosphereContext atmosphereCtx = makeAtmosphereContext(client, frameSettings_, ticks);
 core::fogUpdateFromWorld(client, tickDelta, frameSettings_);
 if(!frameCamera_.shadowPass) {
  core::setFogEnabled(true);
  core::fogApplyMode(client, false, frameSettings_);
 }
 {
  core::clear(gl::attrib::DepthBufferBit |
              (frameCamera_.shadowPass || colorAlreadyCleared ? 0u : gl::attrib::ColorBufferBit));
 }
 renderWorld(tickDelta, fov, modelView, projection);
 {
  math::Matrix4f cameraOnly;
  cameraOnly.identity();
  applyCameraTransform(tickDelta, cameraOnly);
  const float* cv = cameraOnly.data();
  frameCamera_.viewRightX = cv[0];
  frameCamera_.viewRightY = cv[4];
  frameCamera_.viewRightZ = cv[8];
  frameCamera_.viewUpX = cv[1];
  frameCamera_.viewUpY = cv[5];
  frameCamera_.viewUpZ = cv[9];
  frameCamera_.viewForwardX = -cv[2];
  frameCamera_.viewForwardY = -cv[6];
  frameCamera_.viewForwardZ = -cv[10];
  if(frameCamera_.hasExplicitModelView) {
   frameCamera_.eyeX = frameCamera_.x;
   frameCamera_.eyeY = frameCamera_.y;
   frameCamera_.eyeZ = frameCamera_.z;
  } else {
   for(int i = 0; i < 3; ++i) {
    const double e = -(static_cast<double>(cv[i * 4 + 0]) * cv[12] + static_cast<double>(cv[i * 4 + 1]) * cv[13] +
                       static_cast<double>(cv[i * 4 + 2]) * cv[14]);
    (i == 0   ? frameCamera_.eyeX
     : i == 1 ? frameCamera_.eyeY
              : frameCamera_.eyeZ) = (i == 0   ? frameCamera_.x
                                      : i == 1 ? frameCamera_.y
                                               : frameCamera_.z) +
                                     e;
   }
  }
 }
 const PackDefinition& definition = packDefinition();
 if(!frameCamera_.shadowPass) {
  frameCamera_.skipAllRendering = definition.skipAllRendering;
 }
 core::setCameraFrame(frameCamera_);
 core::setDrawCameraStateFromCamera(frameCamera_);
 if(!frameCamera_.shadowPass && client->world != nullptr) {
  updateSunLight(client->world, tickDelta, client->camera);
 }
 if(!frameCamera_.shadowPass && !renderCameraEntity) {
  shaderPipeline_->prepareFrame(client->world);
  shaderPipeline_->setFrameUniforms(buildFrameUniforms(tickDelta));
 }
 if(!frameCamera_.shadowPass && !renderCameraEntity) {
  frameShadow_ = shadowmap::update(shadowState_, *this, tickDelta, frameCamera_, definition);
 }
 if(!frameCamera_.shadowPass && !renderCameraEntity) {
  shaderPipeline_->renderBegin(frameShadow_.depthTexture,
                               frameShadow_.opaqueDepthTexture,
                               frameShadow_.colorTextures.data(),
                               frameShadow_.colorCount,
                               &shadowState_.targets,
                               frameShadow_.colorAltTextures.data());
 }
 Frustum viewFrustum;
 Frustum* activeCuller = nullptr;
 if(!frameCamera_.shadowPass && frameSettings_.frustumCulling && core::drawCameraStateValid()) {
  viewFrustum.compute(core::drawProjection(), core::drawModelView(), frameCamera_.eyeX,
                      frameCamera_.eyeY, frameCamera_.eyeZ);
  activeCuller = &viewFrustum;
 }
 if(!frameCamera_.shadowPass && !frameCamera_.skipAllRendering && client->world->dimension != nullptr &&
    !client->world->dimension->isNether) {
  // Beta renderWorld draws the sky under setupFog(-1) and switches back to setupFog(0)
  // for terrain. Without this the sky was drawn with the terrain's fog range, so it
  // never blended toward the fog colour and the fogged horizon met it as a hard band.
  core::fogApplyMode(client, true, frameSettings_);
  const bool skyRendered = renderWorldStage(atmosphereCtx,
                                            tickDelta,
                                            mod::WorldRenderStage::Sky,
                                            frameSettings_.renderSky,
                                            false,
                                            [&] { atmosphere::renderSkyDome(atmosphereCtx, tickDelta); });
  renderWorldStage(atmosphereCtx,
                   tickDelta,
                   mod::WorldRenderStage::Stars,
                   frameSettings_.renderStars,
                   false,
                   [&](const mod::WorldRenderEvent& event) {
                    atmosphere::renderSkyStars(atmosphereCtx, tickDelta, event.starBrightness);
                   });
  if(skyRendered) {
   atmosphere::renderSkyVoid(atmosphereCtx, tickDelta);
  }
  core::fogApplyMode(client, false, frameSettings_);
  shaderPipeline_->refreshLightmap(client->world);
 }
 if(!frameCamera_.shadowPass && !renderCameraEntity) {
  {
   shaderPipeline_->renderShadowComposite(frameShadow_.depthTexture,
                                          frameShadow_.opaqueDepthTexture,
                                          frameShadow_.colorTextures.data(),
                                          frameShadow_.colorCount,
                                          &shadowState_.targets,
                                          frameShadow_.colorAltTextures.data());
  }
  shaderPipeline_->bindScene();
  {
   shaderPipeline_->renderPreWorld(frameShadow_.depthTexture,
                                   frameShadow_.opaqueDepthTexture,
                                   frameShadow_.colorTextures.data(),
                                   frameShadow_.colorCount,
                                   &shadowState_.targets,
                                   frameShadow_.colorAltTextures.data());
  }
 }
 {
  worldRenderer->sections().cullChunks(activeCuller, !renderCameraEntity);
 }
 if(!renderCameraEntity) {
  worldRenderer->compiler().compileChunks(*camera, false);
 }
 core::WorldLightUniforms worldLight;
 const auto& sun = client->world->lightRegistry().sun();
 worldLight.sunDirWorld[0] = sun.directionX;
 worldLight.sunDirWorld[1] = sun.directionY;
 worldLight.sunDirWorld[2] = sun.directionZ;
 directionToView(sun.directionX, sun.directionY, sun.directionZ, frameCamera_, worldLight.sunDirView);
 worldLight.sunColor[0] = sun.red;
 worldLight.sunColor[1] = sun.green;
 worldLight.sunColor[2] = sun.blue;
 worldLight.sunIntensity = sun.intensity;
 const float skyIntensity = client->world->calculateSkyLightIntensity(tickDelta);
 worldLight.ambient[0] = worldLight.ambient[1] = worldLight.ambient[2] = 0.4f * (1.0f - skyIntensity * 1.5f);
 worldLight.worldTime = static_cast<float>(ticks) + tickDelta;
 core::setWorldLight(worldLight);
 if(frameCamera_.shadowPass) {
  if(frameCamera_.shadowTerrain) {
   drawSolidTerrain(*worldRenderer, *camera, terrainTextureId);
   drawCutoutTerrain(*worldRenderer, *camera, terrainTextureId);
  }
  if(frameCamera_.shadowEntities || frameCamera_.shadowPlayer || frameCamera_.shadowBlockEntities) {
   renderWorldStage(atmosphereCtx, tickDelta, mod::WorldRenderStage::Entities, true, true, [&] {
    core::setLightingEnabled(true);
    const Vec3d shadowCameraPos{frameCamera_.x, frameCamera_.y, frameCamera_.z};
    worldRenderer->renderEntities(shadowCameraPos, activeCuller, tickDelta);
    core::setLightingEnabled(false);
   });
  }
  shadowState_.targets.snapshotOpaqueDepth();
  if(frameCamera_.shadowTranslucent) {
   drawTranslucentTerrain(*worldRenderer, *camera, tickDelta, terrainTextureId, false);
  }
  return;
 }
 const bool skipGbuffers = frameCamera_.skipAllRendering;
 renderWorldStage(atmosphereCtx, tickDelta, mod::WorldRenderStage::OpaqueTerrain, !skipGbuffers, false, [&] {
  drawSolidTerrain(*worldRenderer, *camera, terrainTextureId);
  drawCutoutTerrain(*worldRenderer, *camera, terrainTextureId);
 });
 core::setLightingEnabled(true);
 applyEntityLightingRig(frameCamera_, worldLight);
 const Vec3d frameCameraPos{frameCamera_.x, frameCamera_.y, frameCamera_.z};
 const bool hasDeferred = shaderPipeline_->hasDeferredPasses();
 const bool splitEntities = hasDeferred && definition.separateEntityDraws;
 std::string particleOrder = definition.particleOrdering;
 if(particleOrder.empty()) particleOrder = hasDeferred ? "after" : "mixed";
 renderWorldStage(atmosphereCtx, tickDelta, mod::WorldRenderStage::Entities, !skipGbuffers, false, [&] {
  if(splitEntities) render::setDrawPhase(render::DrawPhase::Opaque);
  worldRenderer->renderEntities(frameCameraPos, activeCuller, tickDelta);
  render::setDrawPhase(render::DrawPhase::All);
 });
 const auto renderLitParticles = [&] {
  core::activeTexture(gl::tex::Texture0);
  client->particleManager.renderLit(camera, tickDelta);
 };
 const auto renderTranslucentParticles = [&] {
  core::setLightingEnabled(false);
  core::setWorldLight(worldLight);
  core::enableBlend();
  core::blendAlpha();
  client->particleManager.render(camera, tickDelta);
 };
 if(!skipGbuffers && (particleOrder == "before" || particleOrder == "mixed")) renderLitParticles();
 {
  shaderPipeline_->captureOpaqueDepth();
 }
 if(zoom == 1.0 && !renderCameraEntity) {
  renderFirstPersonHand(tickDelta);
 }
 {
  shaderPipeline_->captureHandDepth();
 }
 if(!skipGbuffers && particleOrder == "before") renderTranslucentParticles();
 if(hasDeferred) {
  {
   shaderPipeline_->renderDeferred(frameShadow_.depthTexture,
                                   frameShadow_.opaqueDepthTexture,
                                   frameShadow_.colorTextures.data(),
                                   frameShadow_.colorCount,
                                   frameShadow_.colorAltTextures.data());
  }
  shaderPipeline_->bindScene();
 }
 if(splitEntities && !skipGbuffers) {
  // Second full walk of the entity list: DrawPhase filters at submission, so
  // culling, dispatch and model building are paid twice. It belongs to the
  // Entities stage, not to the unattributed remainder of the render phase.
  core::setLightingEnabled(true);
  applyEntityLightingRig(frameCamera_, worldLight);
  render::setDrawPhase(render::DrawPhase::Translucent);
  worldRenderer->renderEntities(frameCameraPos, activeCuller, tickDelta);
  render::setDrawPhase(render::DrawPhase::All);
 }
 if(!skipGbuffers && particleOrder == "after") renderLitParticles();
 if(!skipGbuffers && particleOrder != "before") renderTranslucentParticles();
 if(client->crosshairTarget.has_value()) {
  if(auto* player = dynamic_cast<PlayerEntity*>(camera)) {
   if(camera->isInFluid(material::Material::WATER)) {
    renderBlockOverlay(*worldRenderer, client, *player, tickDelta);
   }
  }
 }
 renderWorldStage(atmosphereCtx, tickDelta, mod::WorldRenderStage::TranslucentTerrain, !skipGbuffers, false, [&] {
  drawTranslucentTerrain(*worldRenderer, *camera, tickDelta, terrainTextureId, fancyGraphics);
 });
 if(!skipGbuffers) {
  shaderPipeline_->sampleCenterDepth();
 }
 if(client->crosshairTarget.has_value() && zoom == 1.0 && !camera->isInFluid(material::Material::WATER)) {
  if(auto* player = dynamic_cast<PlayerEntity*>(camera)) {
   renderBlockOverlay(*worldRenderer, client, *player, tickDelta);
  }
 }
 if(!skipGbuffers) {
  if(frameSettings_.weatherEnabled) {
   atmosphere::renderPrecipitation(atmosphereCtx, tickDelta);
  }
 }
 renderWorldStage(atmosphereCtx,
                  tickDelta,
                  mod::WorldRenderStage::Clouds,
                  !skipGbuffers && frameSettings_.renderClouds && definition.renderClouds,
                  false,
                  [&] {
                   atmosphere::renderClouds(atmosphereCtx, tickDelta);
                  });
 core::cullBackFaces();
 core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
 if(!renderCameraEntity) {
  renderWorldStage(atmosphereCtx, tickDelta, mod::WorldRenderStage::Framebuffer, true, false, [] {});
 }
}
void GameRenderer::renderRain() {
 if(client == nullptr || client->world == nullptr || client->camera == nullptr) {
  return;
 }
 float rain = option::rainGradient(frameSettings_, client->world, 1.0f);
 if(!client->options.fancyGraphics) {
  rain /= 2.0f;
 }
 if(rain == 0.0f) {
  return;
 }
 random.setSeed(static_cast<std::uint64_t>(ticks) * 312987231ULL);
 const auto* living = dynamic_cast<const LivingEntity*>(client->camera);
 if(living == nullptr) {
  return;
 }
 World* world = client->world;
 const int baseX = MathHelper::floor(living->x);
 const int baseY = MathHelper::floor(living->y);
 const int baseZ = MathHelper::floor(living->z);
 constexpr int radius = 10;
 BiomeSource* biomeSource = world->getBiomeSource();
 const int particleCount = static_cast<int>(100.0f * rain * rain);
 for(int i = 0; i < particleCount; ++i) {
  const int px = baseX + random.nextInt(radius) - random.nextInt(radius);
  const int pz = baseZ + random.nextInt(radius) - random.nextInt(radius);
  const int topY = world->getTopSolidBlockY(px, pz);
  if(topY < 0) {
   continue;
  }
  const int belowId = world->getBlockId(px, topY - 1, pz);
  if(topY > baseY + radius || topY < baseY - radius) {
   continue;
  }
  if(biomeSource == nullptr || !biomeSource->getBiome(px, pz).canRain()) {
   continue;
  }
  if(belowId <= 0) {
   continue;
  }
  Block* block = Block::BLOCKS[static_cast<std::size_t>(belowId)];
  if(block == nullptr) {
   continue;
  }
  const float rx = random.nextFloat();
  const float rz = random.nextFloat();
  const double py = static_cast<double>(static_cast<float>(topY) + 0.1f) - block->minY;
  if(&block->material == &material::Material::LAVA) {
   world->addParticle("smoke", static_cast<float>(px) + rx, py, static_cast<float>(pz) + rz, 0.0, 0.0, 0.0);
   continue;
  }
  world->addParticle("rainsplash", static_cast<float>(px) + rx, py, static_cast<float>(pz) + rz, 0.0, 0.0, 0.0);
 }
}
} // namespace net::minecraft::client::render
