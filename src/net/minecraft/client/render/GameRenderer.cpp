#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/camera/GuiProjection.hpp"
#include <charconv>
#include <chrono>
#include <cmath>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/debug/RenderProfiler.hpp"
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
#include "net/minecraft/client/render/pipeline/Manager.hpp"
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
#include "net/minecraft/util/math/MatrixStack.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#ifdef _WIN32
#include "net/minecraft/client/util/DisplayManager.hpp"
#endif
#ifdef MINECRAFT_GL_REAL
#include <GL/glu.h>
#include <algorithm>
#endif
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
void updateSunLight(World* world, float tickDelta) {
 if(world == nullptr) return;
 const float timeOfDay = world->getTime(tickDelta);
 const float angle = timeOfDay * kPiF * 2.0f;
 float sunX = std::sin(0.0f) * std::sin(angle);
 float sunY = std::cos(angle);
 float sunZ = std::cos(0.0f) * std::sin(angle);
 const float length = std::sqrt(sunX * sunX + sunY * sunY + sunZ * sunZ);
 if(length > 0.0001f) {
  sunX /= length;
  sunY /= length;
  sunZ /= length;
 }
 const float daylight = std::clamp((sunY + 0.08f) / 0.28f, 0.0f, 1.0f);
 const float horizon = 1.0f - std::clamp(std::abs(sunY) * 5.0f, 0.0f, 1.0f);
 light::SunLight sun;
 sun.directionX = sunX;
 sun.directionY = sunY;
 sun.directionZ = sunZ;
 sun.red = 1.0f;
 sun.green = 0.96f - horizon * 0.25f;
 sun.blue = 0.88f - horizon * 0.48f;
 sun.intensity = daylight;
 world->lightRegistry().setSun(sun);
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
      shaderPacks_(clientIn != nullptr ? std::make_unique<PackManager>(
                                             Minecraft::getRunDirectory(), &clientIn->options)
                                       : nullptr) {}
GameRenderer::~GameRenderer() {
 shadowmap::reset(shadowState_);
}
PackUniformValues GameRenderer::buildFrameUniforms(float tickDelta,
                                                             float farPlane,
                                                             bool shadowAvailable) const {
 const int width = client != nullptr ? std::max(1, client->displayWidth) : 1;
 const int height = client != nullptr ? std::max(1, client->displayHeight) : 1;
 const float worldTime = static_cast<float>(ticks) + tickDelta;
 const PackDefinition* definition =
     shaderPacks_ != nullptr ? shaderPacks_->activeDefinition() : nullptr;
 const float eyeHalf = definition != nullptr ? definition->eyeBrightnessHalflife : 10.0f;
 return buildShaderFrameData(width, height, farPlane, worldTime,
                             frameShadow_.resolution, shaderPacks_ != nullptr && shaderPacks_->sceneColorCount() > 1,
                             shadowAvailable, frameCamera_, shadowState_.shadowCamera,
                             client != nullptr ? client->world : nullptr, eyeHalf);
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
void GameRenderer::applyDamageTiltEffect(float tickDelta, math::MatrixStack& modelView) {
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
void GameRenderer::applyViewBobbing(float tickDelta, math::MatrixStack& modelView) {
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
void GameRenderer::applyCameraTransform(float tickDelta, math::MatrixStack& modelView) {
 if(client == nullptr || client->camera == nullptr) {
  return;
 }
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/shadows/ShadowRenderer.java
 if(frameCamera_.hasExplicitModelView) {
  math::Matrix4f mv;
  mv.set(frameCamera_.explicitModelView);
  modelView.load(mv);
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
                              math::MatrixStack& modelView,
                              math::MatrixStack& projection) {
 if(client == nullptr) {
  return;
 }
 int viewport[4]{0, 0, client->displayWidth, client->displayHeight};
 if(!core::getCachedViewport(viewport)) {
  core::getIntegerv(gl::query::Viewport, viewport);
 }
 const float aspect = viewport[3] != 0 ? static_cast<float>(viewport[2]) / static_cast<float>(viewport[3]) : 1.0f;
 const option::RenderSettings& resolved = frameSettings_;
 projection.loadIdentity();
  // Far plane = render distance in blocks. The old b1.7.3 2x multiplier wasted
  // the top half of the depth range on geometry that is already fully fogged
  // (fog ends at ~renderDistance), halving depth precision and making
  // coincident / near-coplanar surfaces (crossed plants, fancy leaves) Z-fight.
  // Iris/Java render out to exactly `renderDistance * 16` blocks — match that.
  const float nearPlane = std::max(0.001f, frameCamera_.perspectiveNear);
  const float farPlane = frameCamera_.perspectiveFar > nearPlane
                             ? frameCamera_.perspectiveFar
                             : resolved.renderDistanceBlocks;
 frameCamera_.perspectiveFar = farPlane;
 if(frameCamera_.orthographic) {
  math::Matrix4f proj;
  proj.ortho(-frameCamera_.orthoHalfWidth,
             frameCamera_.orthoHalfWidth,
             -frameCamera_.orthoHalfHeight,
             frameCamera_.orthoHalfHeight,
             frameCamera_.orthoNear,
             frameCamera_.orthoFar);
  projection.load(proj);
 } else {
  if(zoom != 1.0) {
   projection.translate(static_cast<float>(zoomX), static_cast<float>(-zoomY), 0.0f);
   projection.scale(static_cast<float>(zoom), static_cast<float>(zoom), 1.0f);
  }
  math::Matrix4f persp;
  persp.perspective(fov, aspect, nearPlane, farPlane);
  projection.multiply(persp);
 }
 modelView.loadIdentity();
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
  // Iris' bobStack: the damage tilt, view bobbing and nausea distortion are captured
  // as their own matrix here, BEFORE the camera transform, so applyCameraTransform
  // below composes `bob * MV_camera` — bobbing acting in view space after the camera
  // rather than displacing the point the camera sits on. Vanilla applies these to the
  // projection matrix; Iris moves them into the model view because that is what shader
  // packs expect ("need `bob * modelView` not `modelView * bob`").
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/mixin/MixinModelViewBobbing.java
  const math::Matrix4f& bobStack = modelView.top();
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
 const option::RenderSettings& resolved = frameSettings_;
 const float aspect = viewport[3] != 0 ? static_cast<float>(viewport[2]) / static_cast<float>(viewport[3]) : 1.0f;
 math::Matrix4f handProjection;
 if(zoom != 1.0) {
  handProjection.translate(static_cast<float>(zoomX), static_cast<float>(-zoomY), 0.0f);
  handProjection.scale(static_cast<float>(zoom), static_cast<float>(zoom), 1.0f);
 }
 math::Matrix4f handPersp;
  handPersp.perspective(getFov(tickDelta), aspect, 0.05f, resolved.renderDistanceBlocks);
 for(int column = 0; column < 4; ++column) {
  handPersp.m[column * 4 + 2] *= kHandDepth;
 }
 handProjection.multiply(handPersp);
 // The hand renders in camera space with its own projection: the camera state
 // is switched to identity for the duration of the hand pass.
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
  math::MatrixStack modelView;
  applyDamageTiltEffect(tickDelta, modelView);
  if(client->options.bobView) {
   applyViewBobbing(tickDelta, modelView);
  }
  core::setDrawModelView(modelView.top());
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
  // Beta 1.7.3 drew the screen overlays on the plain camera-space base (the
  // tilt/bob compose that followed the overlay draw had no effect).
  heldItemRenderer->renderScreenOverlays(tickDelta);
 }
}
void GameRenderer::onFrameUpdate(float tickDelta) {
 if(client == nullptr) {
  return;
 }
 // Link worker-finished shader binaries into the ProgramCaches before anything
 // renders this frame. Runs even on GUI-only frames (no world) so pack loads and
 // setting changes never stall a frame waiting for a compile.
 if(shaderPacks_ != nullptr) {
  shaderPacks_->pollPrograms();
 }
 const PackDefinition* pack =
     shaderPacks_ != nullptr ? shaderPacks_->meshDefinition() : nullptr;
 frameSettings_ = option::renderSettings(client->options, pack);
 if(client->worldRenderer != nullptr) client->worldRenderer->setRenderSettings(frameSettings_);
#ifdef _WIN32
 if(!util::DisplayManager::isActive()) {
#else
 if(!client->focused.load()) {
#endif
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
 if(client->world != nullptr) {
  renderFrame(tickDelta);
  if(!client->options.hideHud || client->currentScreen() != nullptr) {
    if(shaderPacks_ != nullptr) {
     shaderPacks_->pipeline().setPipelinePhase(WorldPipelinePhase::None);
    }
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
   if(shaderPacks_ != nullptr) {
    shaderPacks_->pipeline().setPipelinePhase(WorldPipelinePhase::None);
   }
  input::InputSystem& input = input::InputSystem::instance();
#ifdef _WIN32
  input.syncCursorFromOs();
#endif
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
bool GameRenderer::beginSceneCapture() {
 if(client == nullptr || shaderPacks_ == nullptr) {
  return false;
 }
 shaderPacks_->poll();
 const int width = std::max(1, client->displayWidth);
 const int height = std::max(1, client->displayHeight);
 if(!shaderPacks_->ensureSceneTargets(width, height)) {
  return false;
 }
 shaderPacks_->clearScene(core::fog().color[0], core::fog().color[1], core::fog().color[2]);
 core::clear(gl::attrib::DepthBufferBit);
 return true;
}
void GameRenderer::resolveSceneCapture() {
 if(client == nullptr || shaderPacks_ == nullptr) {
  return;
 }
 shaderPacks_->endScene();
 const int width = std::max(1, client->displayWidth);
 const int height = std::max(1, client->displayHeight);
 gl::GLCore::bindFramebuffer(static_cast<unsigned>(gl::framebuffer::Framebuffer), 0);
 core::viewport(0, 0, width, height);
 core::clear(gl::attrib::ColorBufferBit | gl::attrib::DepthBufferBit);
 shaderPacks_->renderPostProcess(frameShadow_.depthTexture, frameShadow_.opaqueDepthTexture,
                                 frameShadow_.colorTextures.data(), frameShadow_.colorCount,
                                 frameShadow_.colorAltTextures.data());
 shaderPacks_->pipeline().reset();
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
                                                      LivingEntity* camera,
                                                      const option::RenderSettings& settings,
                                                      int atmosphereTicks) {
 return AtmosphereContext{
     .client = client,
     .world = client->world,
     .textureManager = &client->textureManager,
     .camera = client->camera,
     .livingCamera = camera,
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
struct ProfilerFrame {
 bool active;
 explicit ProfilerFrame(bool enabled) : active(enabled) {
  if(active) {
   debug::RenderProfiler::instance().beginFrame();
  }
 }
 ProfilerFrame(const ProfilerFrame&) = delete;
 ProfilerFrame& operator=(const ProfilerFrame&) = delete;
 ~ProfilerFrame() {
  if(active) {
   debug::RenderProfiler::instance().endFrame();
  }
 }
};
void drawSolidTerrain(WorldRenderer& worldRenderer,
                      LivingEntity& camera,
                      float tickDelta,
                      int terrainTextureId) {
 bindTerrainTexture(terrainTextureId);
 const RenderPassScope solidScope(RenderType::solid());
 core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
 worldRenderer.render(camera, chunk::terrain_layer::Solid, static_cast<double>(tickDelta));
}
void drawCutoutTerrain(WorldRenderer& worldRenderer,
                       LivingEntity& camera,
                       float tickDelta,
                       int terrainTextureId) {
 bindTerrainTexture(terrainTextureId);
 const RenderPassScope cutoutScope(RenderType::cutout());
 core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
 worldRenderer.render(camera, chunk::terrain_layer::Cutout, static_cast<double>(tickDelta));
}
// TEMP DIAGNOSTIC — water/ice not drawing under RenderPearl/SEUS.
// See docs/agent-notes/HANDOFF-water-ice-not-drawing.md. DELETE THIS FUNCTION AND ITS
// CALL below once the cause is found.
// frameCounter must be a DISTINCT static per call site: a shared counter increments
// once per call, so with two call sites per frame the `% N` gate only ever lands on one
// of them (even/odd aliasing) and the other pass silently never reports.
void logTranslucentPassState(const char* whenLabel, int drawnRegions, int& frameCounter) {
 // First 3 frames, then every 60th, so a session yields a readable trace not a flood.
 const int current = frameCounter++;
 if(current >= 3 && current % 60 != 0) {
  return;
 }
 const gl::ShaderProgram* program = core::program();
 int boundFbo = 0;
 ::glGetIntegerv(0x8CA6, &boundFbo); // GL_DRAW_FRAMEBUFFER_BINDING
 std::string drawBufferList;
 for(int i = 0; i < 4; ++i) {
  int value = 0;
  ::glGetIntegerv(static_cast<unsigned>(0x8825 + i), &value); // GL_DRAW_BUFFER0 + i
  if(value == 0) continue; // GL_NONE
  if(!drawBufferList.empty()) drawBufferList += ",";
  // 0x8CE0 == GL_COLOR_ATTACHMENT0
  drawBufferList += "DB" + std::to_string(i) + "=attach" + std::to_string(value - 0x8CE0);
 }
 if(drawBufferList.empty()) drawBufferList = "NONE";
 // Read the SEPARATE alpha factors straight from GL. core::blendSrcFactor/blendDstFactor
 // only mirror the RGB pair, so a pack's `blend.<program>=RGBsrc RGBdst Asrc Adst`
 // (RenderPearl water uses ZERO ZERO for alpha) is invisible in the cached values.
 int blendSrcAlpha = 0;
 int blendDstAlpha = 0;
 ::glGetIntegerv(0x80CB, &blendSrcAlpha); // GL_BLEND_SRC_ALPHA
 ::glGetIntegerv(0x80CA, &blendDstAlpha); // GL_BLEND_DST_ALPHA
 ClientLog::LOGGER.log(
     ::net::minecraft::util::logging::LogLevel::Info,
     std::string("[water-probe] ") +
         (RenderCameraState::instance().frame().shadowPass ? "SHADOW " : "WORLD  ") + whenLabel + " program=" +
         (program == nullptr ? std::string("NULL") : std::to_string(program->handle())) +
         " drawEnabled=" + (core::drawEnabled() ? "1" : "0") + " blend=" +
         (core::blendEnabled() ? "1" : "0") + " src=" + std::to_string(core::blendSrcFactor()) + " dst=" +
         std::to_string(core::blendDstFactor()) + " srcA=" + std::to_string(blendSrcAlpha) + " dstA=" +
         std::to_string(blendDstAlpha) + " alphaRef=" + std::to_string(core::alphaTestRef()) +
         " depthWrite=" + (core::depthWriteEnabled() ? "1" : "0") + " fbo=" + std::to_string(boundFbo) +
         " drawBuffers=[" + drawBufferList + "] layer2Draws=" + std::to_string(drawnRegions));
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
    worldRenderer.render(camera, chunk::terrain_layer::Translucent, static_cast<double>(tickDelta), false);
   }
   static int prepassFrames = 0;                                                            // TEMP DIAGNOSTIC
   logTranslucentPassState("depth-prepass", worldRenderer.lastDrawnRegions(), prepassFrames); // TEMP DIAGNOSTIC
   worldRenderer.renderLastChunks(chunk::terrain_layer::Translucent, static_cast<double>(tickDelta));
   static int colorFrames = 0;                                                              // TEMP DIAGNOSTIC
   logTranslucentPassState("color-pass   ", worldRenderer.lastDrawnRegions(), colorFrames);  // TEMP DIAGNOSTIC
  } else {
   worldRenderer.render(camera, chunk::terrain_layer::Translucent, static_cast<double>(tickDelta));
   static int singleFrames = 0;                                                             // TEMP DIAGNOSTIC
   logTranslucentPassState("single-pass  ", worldRenderer.lastDrawnRegions(), singleFrames); // TEMP DIAGNOSTIC
  }
 }
 core::depthMask(true);
 core::cullBackFaces();
}
void renderBlockOverlay(WorldRenderer& worldRenderer,
                        Minecraft* client,
                        PlayerEntity& player,
                        float tickDelta) {
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
 mod::WorldRenderEvent event{
     context.world,
     context.camera,
     tickDelta,
     stage,
     mod::RenderHookMoment::Before,
 };
 event.shadowPass = shadowPass;
 event.excludedEntityId = -1;
 mod::runtime::luaHookWorldRender(event);
 if(enabled && !event.cancelVanilla) {
  draw();
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
 const bool captured = beginSceneCapture();
 const int width = std::max(1, client->displayWidth);
 const int height = std::max(1, client->displayHeight);
 renderToCurrentTarget(tickDelta, FrameRenderCamera{}, getFov(tickDelta), width, height, false);
 if(captured) {
  resolveSceneCapture();
 }
}
bool GameRenderer::renderWorldToFbo(unsigned int fbo,
                                    int width,
                                    int height,
                                    float tickDelta,
                                    const FrameRenderCamera& camera,
                                    float fov,
                                    FrameRenderCamera* outCamera) {
 if(client == nullptr || fbo == 0 || width <= 0 || height <= 0 || !std::isfinite(tickDelta) ||
    !std::isfinite(fov)) {
  return false;
 }
 int prevFbo = 0;
 int prevViewport[4] = {0, 0, 0, 0};
 ::glGetIntegerv(0x8CA6, &prevFbo);
 core::getIntegerv(0x0BA2, prevViewport);
 const core::BlendScope blendGuard(core::blendEnabled());
 const core::DepthScope depthGuard(core::depthTestEnabled(), core::depthWriteEnabled());
 const core::CullScope cullGuard(core::cullEnabled());
 const core::TextureBindScope textureGuard;
 const core::ScopedDrawCameraState drawCameraGuard;
 const FrameRenderCamera prevFrameCamera = frameCamera_;
 const FrameRenderCamera prevPublishedCamera = RenderCameraState::instance().frame();
 WorldRenderer* worldRenderer = client->worldRenderer.get();
 Entity* prevWorldCam = nullptr;
 bool prevRenderCamEntity = false;
 if(worldRenderer != nullptr) {
  prevWorldCam = worldRenderer->cameraEntity_;
  prevRenderCamEntity = worldRenderer->renderCameraEntity_;
  worldRenderer->pushCullState();
 }
 gl::GLCore::bindFramebuffer(gl::framebuffer::Framebuffer, fbo);
 core::viewport(0, 0, width, height);
 renderToCurrentTarget(std::clamp(tickDelta, 0.0f, 1.0f), camera, std::clamp(fov, 1.0f, 179.0f), width,
                       height, true);
 if(outCamera != nullptr) {
  *outCamera = frameCamera_;
 }
 if(worldRenderer != nullptr) {
  worldRenderer->cameraEntity_ = prevWorldCam;
  worldRenderer->renderCameraEntity_ = prevRenderCamEntity;
  worldRenderer->popCullState();
 }
 frameCamera_ = prevFrameCamera;
 RenderCameraState::instance().setFrame(prevPublishedCamera);
 gl::GLCore::bindFramebuffer(gl::framebuffer::Framebuffer, static_cast<unsigned>(prevFbo));
 core::viewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
 return true;
}
void GameRenderer::renderToCurrentTarget(float tickDelta,
                                         const FrameRenderCamera& cameraFrame,
                                         float fov,
                                         int viewportWidth,
                                         int viewportHeight,
                                         bool renderCameraEntity) {
 if(client == nullptr) {
  return;
 }
 const PackManager::PipelinePhaseScope pipelinePhase(
     shaderPacks_.get(),
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
 math::MatrixStack modelView;
 math::MatrixStack projection;
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
 const option::RenderSettings& resolvedOptions = frameSettings_;
 const bool fancyGraphics = client->options.fancyGraphics;
 const int terrainTextureId = client->textureManager.getTextureId("/terrain.png");
  const AtmosphereContext atmosphereCtx = makeAtmosphereContext(client, camera, frameSettings_, ticks);
  core::fogUpdateFromWorld(client, tickDelta, frameSettings_);
  // Resolve the environmental fog (mode/start/end/density) BEFORE the frame
  // uniforms are snapshotted below (setFrameUniforms), so shader packs sample the
  // same fog the per-draw passes capture instead of stale/off state.
  // Iris FogUniforms reads the captured fog state per frame (fogMode/fogDensity
  // from CapturedRenderingState, fogStart/fogEnd from the sodium FogStorage);
  // this port snapshots render::core::fog() once per frame in buildFrameUniforms.
  // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/uniforms/FogUniforms.java
  if(!frameCamera_.shadowPass) {
   core::setFogEnabled(true);
   core::fogApplyMode(client, 1, frameSettings_);
  }
  debug::RenderProfiler::instance().setEnabled(client->options.debugHud);
 const ProfilerFrame profilerFrame(client->options.debugHud && !frameCamera_.shadowPass && !renderCameraEntity);
 core::clear(gl::attrib::ColorBufferBit | gl::attrib::DepthBufferBit);
 renderWorld(tickDelta, fov, modelView, projection);
 {
  const float* p = projection.top().data();
  frameCamera_.projectionX = p[0];
  frameCamera_.projectionY = p[5];
 }
 // The camera matrix ALONE — no bobbing, no damage tilt, no nausea. Those were
 // captured separately in renderWorld as Iris' bobStack and are left-multiplied on
 // top, so they must not contaminate what we recover here: this matrix defines Java's
 // Camera.getPosition() (the single geometry origin) and the camera rotation basis.
 // Recovering them from the composed `bob * MV_camera` instead would fold the bob into
 // the origin, which is what makes shadows and world-space reconstruction drift.
 // https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/mixin/MixinModelViewBobbing.java
 {
  math::MatrixStack cameraOnly;
  cameraOnly.loadIdentity();
  applyCameraTransform(tickDelta, cameraOnly);
  const float* cv = cameraOnly.top().data();
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
   // The caller states the origin outright (ShadowMapPass centres the shadow camera
   // on the gbuffer camera position). The matrix's own translation is the
   // shadowIntervalSize grid snap, which belongs in the model view — back-deriving
   // an origin from it would apply the snap twice.
   frameCamera_.eyeX = frameCamera_.x;
   frameCamera_.eyeY = frameCamera_.y;
   frameCamera_.eyeZ = frameCamera_.z;
  } else {
   // MV_camera = R * T(pos - eye), so eye = pos - R^T * translation.
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
 const PackDefinition* definition =
     shaderPacks_ != nullptr ? shaderPacks_->activeDefinition() : nullptr;
 if(!frameCamera_.shadowPass) {
  frameCamera_.skipAllRendering = definition != nullptr && definition->skipAllRendering;
 }
 RenderCameraState::instance().setFrame(frameCamera_);
  const float farPlane = frameCamera_.perspectiveFar > frameCamera_.perspectiveNear
                             ? frameCamera_.perspectiveFar
                             : frameSettings_.renderDistanceBlocks;
 core::setDrawCameraStateFromCamera(frameCamera_, farPlane);
 const bool skyWillCommit =
     !frameCamera_.shadowPass && resolvedOptions.renderSky &&
     client->world->dimension != nullptr && !client->world->dimension->isNether;
 if(!skyWillCommit) {
  updateSunLight(client->world, tickDelta);
 }
  if(!frameCamera_.shadowPass && !renderCameraEntity && shaderPacks_ != nullptr) {
   shaderPacks_->prepareFrame(client->world);
   shaderPacks_->setFrameUniforms(buildFrameUniforms(tickDelta, farPlane, frameShadow_.depthTexture >= 0));
   // Java runs the BEGIN stage before the shadow map render (onBeginClear →
   // beginRenderer.renderAll, IrisRenderingPipeline.java:1022; renderShadows runs
   // later), so begin programs sample the shadow targets as they exist at the start
   // of the frame — the previous frame's maps (frameShadow_).
   shaderPacks_->renderBegin(frameShadow_.depthTexture,
                             frameShadow_.opaqueDepthTexture,
                             frameShadow_.colorTextures.data(),
                             frameShadow_.colorCount,
                             &shadowState_.targets,
                             frameShadow_.colorAltTextures.data());
  }
  if(!frameCamera_.shadowPass && !renderCameraEntity && shaderPacks_ != nullptr) {
   frameShadow_ = shadowmap::update(shadowState_, *this, tickDelta, frameCamera_, definition);
  } else if(!frameCamera_.shadowPass && !renderCameraEntity) {
   frameShadow_ = {};
  }
  if(client->options.frustumCulling && resolvedOptions.frustumCulling) {
   Frustum::getInstance().compute();
  }
  core::setFogEnabled(!frameCamera_.shadowPass);
  if(!frameCamera_.shadowPass && !frameCamera_.skipAllRendering &&
     resolvedOptions.renderSky &&
     (definition == nullptr || definition->renderSky)) {
    const debug::RenderProfiler::Scope skyScope(debug::RenderStage::Sky);
    // Vanilla draws the sky dome with the same world fog as the terrain so the
    // horizon fades into the fog colour; the sky must use the terrain start/end,
    // not a start-at-zero override that washes the whole dome toward fog.
    core::fogApplyMode(client, 0, frameSettings_);
   if(client->world->dimension != nullptr && !client->world->dimension->isNether) {
    atmosphere::renderSkyDome(atmosphereCtx, tickDelta);
     if(shaderPacks_ != nullptr) {
      shaderPacks_->pipeline().refreshLightmap(client->world);
     }
   }
  }
   if(!frameCamera_.shadowPass && !renderCameraEntity && shaderPacks_ != nullptr) {
    // Java order: shadow map render → shadow composite stage → PREPARE (ShadowRenderer
    // renderShadows → compositeRenderer.renderAll, then prepareRenderer.renderAll,
    // IrisRenderingPipeline.java:1028-1034).
    shaderPacks_->renderShadowComposite(frameShadow_.depthTexture,
                                        frameShadow_.opaqueDepthTexture,
                                        frameShadow_.colorTextures.data(),
                                        frameShadow_.colorCount,
                                        &shadowState_.targets,
                                        frameShadow_.colorAltTextures.data());
    shaderPacks_->bindScene();
    shaderPacks_->renderPreWorld(frameShadow_.depthTexture,
                                 frameShadow_.opaqueDepthTexture,
                                 frameShadow_.colorTextures.data(),
                                 frameShadow_.colorCount,
                                 &shadowState_.targets,
                                 frameShadow_.colorAltTextures.data());
   }
 if(!frameCamera_.shadowPass) {
  core::fogApplyMode(client, 1, frameSettings_);
 }
 FrustumCuller frustumCuller;
 FrustumCuller* activeCuller = nullptr;
 if(client->options.frustumCulling && resolvedOptions.frustumCulling) {
   frustumCuller.prepare(frameCamera_.eyeX, frameCamera_.eyeY, frameCamera_.eyeZ);
  activeCuller = &frustumCuller;
 }
 {
  const debug::RenderProfiler::Scope cullScope(debug::RenderStage::Cull);
  worldRenderer->cullChunks(activeCuller, tickDelta, !renderCameraEntity);
 }
 if(!renderCameraEntity) {
  const debug::RenderProfiler::Scope compileScope(debug::RenderStage::Compile);
  worldRenderer->compileChunks(*camera, false);
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
 worldLight.brightness = client->options.brightness;
 core::setWorldLight(worldLight);
 if(frameCamera_.shadowPass) {
  core::setFogEnabled(false);
  if(frameCamera_.shadowTerrain) {
   drawSolidTerrain(*worldRenderer, *camera, tickDelta, terrainTextureId);
   drawCutoutTerrain(*worldRenderer, *camera, tickDelta, terrainTextureId);
  }
  if(frameCamera_.shadowEntities || frameCamera_.shadowPlayer || frameCamera_.shadowBlockEntities) {
   renderWorldStage(atmosphereCtx, tickDelta, mod::WorldRenderStage::Entities, true, true, [&] {
    core::setLightingEnabled(true);
    const Vec3d shadowCameraPos{frameCamera_.x, frameCamera_.y, frameCamera_.z};
    worldRenderer->renderEntities(shadowCameraPos, activeCuller, tickDelta);
    core::setLightingEnabled(false);
   });
  }
  shadowmap::snapshotOpaqueDepth(shadowState_);
  if(frameCamera_.shadowTranslucent) {
   drawTranslucentTerrain(*worldRenderer, *camera, tickDelta, terrainTextureId, fancyGraphics);
  }
  return;
 }
 const bool skipGbuffers = frameCamera_.skipAllRendering;
 core::fogApplyMode(client, 0, frameSettings_);
 renderWorldStage(atmosphereCtx, tickDelta, mod::WorldRenderStage::OpaqueTerrain, !skipGbuffers, false, [&] {
  const debug::RenderProfiler::Scope solidScope(debug::RenderStage::SolidTerrain);
  drawSolidTerrain(*worldRenderer, *camera, tickDelta, terrainTextureId);
  drawCutoutTerrain(*worldRenderer, *camera, tickDelta, terrainTextureId);
 });
 core::setLightingEnabled(true);
 applyEntityLightingRig(frameCamera_, worldLight);
 const Vec3d frameCameraPos{frameCamera_.x, frameCamera_.y, frameCamera_.z};
 const bool hasDeferred = shaderPacks_ != nullptr && shaderPacks_->hasDeferredPasses();
 const bool splitEntities = hasDeferred && definition != nullptr && definition->separateEntityDraws;
 std::string particleOrder = definition != nullptr ? definition->particleOrdering : std::string{};
 if(particleOrder.empty()) particleOrder = hasDeferred ? "after" : "mixed";
 renderWorldStage(atmosphereCtx, tickDelta, mod::WorldRenderStage::Entities, !skipGbuffers, false, [&] {
  const debug::RenderProfiler::Scope entityScope(debug::RenderStage::Entities);
  if(splitEntities) render::setDrawPhase(render::DrawPhase::Opaque);
   worldRenderer->renderEntities(frameCameraPos, activeCuller, tickDelta);
   render::setDrawPhase(render::DrawPhase::All);
  });
  const auto renderLitParticles = [&] {
  const debug::RenderProfiler::Scope particleScope(debug::RenderStage::Particles);
  core::activeTexture(gl::tex::Texture0);
  client->particleManager.renderLit(camera, tickDelta);
 };
 const auto renderTranslucentParticles = [&] {
  const debug::RenderProfiler::Scope particleScope(debug::RenderStage::Particles);
  core::setLightingEnabled(false);
  core::setWorldLight(worldLight);
  core::fogApplyMode(client, 0, frameSettings_);
  core::enableBlend();
  core::blendAlpha();
  client->particleManager.render(camera, tickDelta);
 };
  if(!skipGbuffers && (particleOrder == "before" || particleOrder == "mixed")) renderLitParticles();
  if(shaderPacks_ != nullptr) {
   shaderPacks_->captureOpaqueDepth();
  }
  if(zoom == 1.0 && !renderCameraEntity) {
   const debug::RenderProfiler::Scope handScope(debug::RenderStage::Hand);
   renderFirstPersonHand(tickDelta);
  }
  if(shaderPacks_ != nullptr) {
   shaderPacks_->captureHandDepth();
  }
  if(!skipGbuffers && particleOrder == "before") renderTranslucentParticles();
   if(hasDeferred) {
    shaderPacks_->renderDeferred(frameShadow_.depthTexture,
                                 frameShadow_.opaqueDepthTexture,
                                 frameShadow_.colorTextures.data(),
                                 frameShadow_.colorCount,
                                 frameShadow_.colorAltTextures.data());
    shaderPacks_->bindScene();
   }
 if(splitEntities && !skipGbuffers) {
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
 core::fogApplyMode(client, 0, frameSettings_);
 renderWorldStage(atmosphereCtx, tickDelta, mod::WorldRenderStage::TranslucentTerrain, !skipGbuffers, false, [&] {
  const debug::RenderProfiler::Scope translucentScope(debug::RenderStage::TranslucentTerrain);
  drawTranslucentTerrain(*worldRenderer, *camera, tickDelta, terrainTextureId, fancyGraphics);
  });
  if(!skipGbuffers && shaderPacks_ != nullptr) {
   shaderPacks_->sampleCenterDepth();
  }
 if(client->crosshairTarget.has_value() && zoom == 1.0 &&
    !camera->isInFluid(material::Material::WATER)) {
  if(auto* player = dynamic_cast<PlayerEntity*>(camera)) {
   renderBlockOverlay(*worldRenderer, client, *player, tickDelta);
  }
 }
  if(!skipGbuffers) {
   if(resolvedOptions.weatherEnabled) {
    precipitationRenderer.renderPrecipitation(atmosphereCtx, tickDelta);
   }
  }
  renderWorldStage(atmosphereCtx,
                  tickDelta,
                  mod::WorldRenderStage::Clouds,
                  !skipGbuffers && resolvedOptions.renderClouds &&
                      (definition == nullptr || definition->renderClouds),
                  false,
                  [&] {
                   const debug::RenderProfiler::Scope cloudScope(debug::RenderStage::Clouds);
                   cloudRenderer.renderClouds(atmosphereCtx, tickDelta);
                  });
 core::setFogEnabled(false);
 core::fogApplyMode(client, 1, frameSettings_);
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
