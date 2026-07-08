#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/client/gui/screen/ChatScreen.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereContext.hpp"
#include "net/minecraft/client/render/atmosphere/SkyDome.hpp"
#include "net/minecraft/client/render/culling/Frustum.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/client/util/UiScale.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/mod/GameHooks.hpp"
#include "net/minecraft/util/hit/HitResult.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#ifdef _WIN32
#include "net/minecraft/client/util/DisplayManager.hpp"
#endif
#ifdef MINECRAFT_GL_REAL
#include <GL/glu.h>
#endif
#include <chrono>
#include <cmath>
#include <optional>
#include <thread>
#include <vector>
namespace net::minecraft::client::render {
namespace option = net::minecraft::client::option;
namespace {
constexpr int kBedBlockId = 26;
constexpr float kPiF = 3.14159265f;
void gluPerspectiveFov(float fovyDeg, float aspect, float zNear, float zFar) {
#ifdef MINECRAFT_GL_REAL
  ::gluPerspective(static_cast<GLdouble>(fovyDeg), static_cast<GLdouble>(aspect), static_cast<GLdouble>(zNear),
                   static_cast<GLdouble>(zFar));
#else
  (void)fovyDeg;
  (void)aspect;
  (void)zNear;
  (void)zFar;
#endif
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
  Vec3d dir{end.x - start.x, end.y - start.y, end.z - start.z};
  double tMin = 0.0;
  double tMax = 1.0;
  for(int axis = 0; axis < 3; ++axis) {
    const double s = axis == 0 ? start.x : (axis == 1 ? start.y : start.z);
    const double d = axis == 0 ? dir.x : (axis == 1 ? dir.y : dir.z);
    const double bMin = axis == 0 ? box.minX : (axis == 1 ? box.minY : box.minZ);
    const double bMax = axis == 0 ? box.maxX : (axis == 1 ? box.maxY : box.maxZ);
    if(std::abs(d) < 1.0e-7) {
      if(s < bMin || s > bMax) {
        return std::nullopt;
      }
      continue;
    }
    double t1 = (bMin - s) / d;
    double t2 = (bMax - s) / d;
    if(t1 > t2) {
      std::swap(t1, t2);
    }
    tMin = std::max(tMin, t1);
    tMax = std::min(tMax, t2);
    if(tMin > tMax) {
      return std::nullopt;
    }
  }
  const double tHit = tMin >= 0.0 ? tMin : tMax;
  if(tHit < 0.0 || tHit > 1.0) {
    return std::nullopt;
  }
  return HitResult{MathHelper::floor(start.x + dir.x * tHit),
                   MathHelper::floor(start.y + dir.y * tHit),
                   MathHelper::floor(start.z + dir.z * tHit),
                   0,
                   {start.x + dir.x * tHit, start.y + dir.y * tHit, start.z + dir.z * tHit}};
}
[[nodiscard]] float worldBrightness(World* world, int x, int y, int z) {
  if(world == nullptr) {
    return 1.0f;
  }
  // GameRenderer.updateCamera — World.getLightBrightness (0..1 via lightLevelToLuminance).
  return world->getLightBrightness(x, y, z);
}
[[nodiscard]] float worldRainGradient(Minecraft* client, World* world, float tickDelta) {
  if(client == nullptr) {
    return world != nullptr ? world->getRainGradient(tickDelta) : 0.0f;
  }
  return client::option::rainGradient(client::option::resolve(client->options), world, tickDelta);
}
[[nodiscard]] std::optional<HitResult> entityRaycast(World* world, LivingEntity* camera, double reach, float tickDelta) {
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
[[nodiscard]] std::int64_t nowNanos() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
      .count();
}
void sleepMillis(std::int64_t ms) {
  if(ms > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  }
}
} // namespace
GameRenderer::GameRenderer(net::minecraft::client::Minecraft* clientIn)
    : client(clientIn), heldItemRenderer(std::make_unique<item::HeldItemRenderer>(clientIn)),
      lastInactiveTime(nowMillis()) {
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
    ambient = worldBrightness(client->world, MathHelper::floor(client->camera->x),
                              MathHelper::floor(client->camera->y), MathHelper::floor(client->camera->z));
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
  if(living->isInFluid(::net::minecraft::block::material::Material::WATER)) {
    fov = 60.0f;
  }
  if(living->health <= 0) {
    const float death = static_cast<float>(living->deathTime) + tickDelta;
    fov /= (1.0f - 500.0f / (death + 500.0f)) * 2.0f + 1.0f;
  }
  fov = option::adjustFieldOfView(fov, option::resolve(client->options));
  mod::FovEvent event{living, tickDelta, fov};
  mod::hooks().publish(event);
  return event.fov + prevCameraRoll + (cameraRoll - prevCameraRoll) * tickDelta;
}
void GameRenderer::applyDamageTiltEffect(float tickDelta) {
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
    gl::rotatef(40.0f - 8000.0f / (death + 200.0f), 0.0f, 0.0f, 1.0f);
  }
  if(hurt < 0.0f) {
    return;
  }
  hurt /= static_cast<float>(living->damagedTime);
  hurt = MathHelper::sin(hurt * hurt * hurt * hurt * kPiF);
  const float swing = living->damagedSwingDir;
  gl::rotatef(-swing, 0.0f, 1.0f, 0.0f);
  gl::rotatef(-hurt * 14.0f, 0.0f, 0.0f, 1.0f);
  gl::rotatef(swing, 0.0f, 1.0f, 0.0f);
}
void GameRenderer::applyViewBobbing(float tickDelta) {
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
  gl::translatef(MathHelper::sin(phase * kPiF) * stepBob * 0.5f,
                 -std::abs(MathHelper::cos(phase * kPiF) * stepBob), 0.0f);
  gl::rotatef(MathHelper::sin(phase * kPiF) * stepBob * 3.0f, 0.0f, 0.0f, 1.0f);
  gl::rotatef(std::abs(MathHelper::cos(phase * kPiF - 0.2f) * stepBob) * 5.0f, 1.0f, 0.0f, 0.0f);
  gl::rotatef(tiltBob, 1.0f, 0.0f, 0.0f);
}
void GameRenderer::applyCameraTransform(float tickDelta) {
  if(client == nullptr || client->camera == nullptr) {
    return;
  }
  auto* living = dynamic_cast<LivingEntity*>(client->camera);
  if(living == nullptr) {
    return;
  }
  if(frameCamera_.customView) {
    gl::rotatef(frameCamera_.roll, 0.0f, 0.0f, 1.0f);
    gl::rotatef(frameCamera_.pitch, 1.0f, 0.0f, 0.0f);
    gl::rotatef(frameCamera_.yaw + 180.0f, 0.0f, 1.0f, 0.0f);
    return;
  }
  float eyeOffset = living->standingEyeHeight - 1.62f;
  double interpX = living->prevX + (living->x - living->prevX) * static_cast<double>(tickDelta);
  double interpY =
      living->prevY + (living->y - living->prevY) * static_cast<double>(tickDelta) - static_cast<double>(eyeOffset);
  double interpZ = living->prevZ + (living->z - living->prevZ) * static_cast<double>(tickDelta);
  gl::rotatef(prevCameraRollAmount + (cameraRollAmount - prevCameraRollAmount) * tickDelta, 0.0f, 0.0f, 1.0f);
  if(living->isSleeping()) {
    eyeOffset += 1.0f;
    gl::translatef(0.0f, 0.3f, 0.0f);
    if(!client->options.debugCamera && client->world != nullptr) {
      const int blockId = client->world->getBlockId(MathHelper::floor(living->x), MathHelper::floor(living->y),
                                                    MathHelper::floor(living->z));
      if(blockId == kBedBlockId) {
        const int meta = client->world->getBlockMeta(MathHelper::floor(living->x), MathHelper::floor(living->y),
                                                     MathHelper::floor(living->z));
        const int facing = static_cast<int>(meta) & 3;
        gl::rotatef(static_cast<float>(facing) * 90.0f, 0.0f, 1.0f, 0.0f);
      }
      gl::rotatef(living->prevYaw + (living->yaw - living->prevYaw) * tickDelta + 180.0f, 0.0f, -1.0f,
                  0.0f);
      gl::rotatef(living->prevPitch + (living->pitch - living->prevPitch) * tickDelta, -1.0f, 0.0f, 0.0f);
    }
  } else if(client->options.thirdPerson) {
    double camDist =
        prevThirdPersonDistance + (thirdPersonDistance - prevThirdPersonDistance) * static_cast<double>(tickDelta);
    if(client->options.debugCamera) {
      const float dbgYaw = prevThirdPersonYaw + (thirdPersonYaw - prevThirdPersonYaw) * tickDelta;
      const float dbgPitch = prevThirdPersonPitch + (thirdPersonPitch - prevThirdPersonPitch) * tickDelta;
      gl::translatef(0.0f, 0.0f, static_cast<float>(-camDist));
      gl::rotatef(dbgPitch, 1.0f, 0.0f, 0.0f);
      gl::rotatef(dbgYaw, 0.0f, 1.0f, 0.0f);
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
        const Vec3d rayStart{interpX + static_cast<double>(sx), interpY + static_cast<double>(sy),
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
      gl::rotatef(living->pitch - basePitch, 1.0f, 0.0f, 0.0f);
      gl::rotatef(living->yaw - baseYaw, 0.0f, 1.0f, 0.0f);
      gl::translatef(0.0f, 0.0f, static_cast<float>(-camDist));
      gl::rotatef(baseYaw - living->yaw, 0.0f, 1.0f, 0.0f);
      gl::rotatef(basePitch - living->pitch, 1.0f, 0.0f, 0.0f);
    }
  } else {
    gl::translatef(0.0f, 0.0f, -0.1f);
  }
  if(!client->options.debugCamera) {
    gl::rotatef(living->prevPitch + (living->pitch - living->prevPitch) * tickDelta, 1.0f, 0.0f, 0.0f);
    gl::rotatef(living->prevYaw + (living->yaw - living->prevYaw) * tickDelta + 180.0f, 0.0f, 1.0f, 0.0f);
  }
  gl::translatef(0.0f, eyeOffset, 0.0f);
}
void GameRenderer::updateSkyAndFogColors(float tickDelta) {
  if(client == nullptr || client->world == nullptr || client->camera == nullptr) {
    return;
  }
  World& world = *client->world;
  const option::ResolvedRenderOptions resolved = option::resolve(client->options);
  const Vec3d sky = world.getSkyColor(client->camera, tickDelta);
  const Vec3d fog = world.getFogColor(tickDelta);
  fogRed = static_cast<float>(fog.x + (sky.x - fog.x) * resolved.fogColorBlend);
  fogGreen = static_cast<float>(fog.y + (sky.y - fog.y) * resolved.fogColorBlend);
  fogBlue = static_cast<float>(fog.z + (sky.z - fog.z) * resolved.fogColorBlend);
  const float rain = option::rainGradient(resolved, &world, tickDelta);
  if(rain > 0.0f) {
    fogRed *= 1.0f - rain * 0.5f;
    fogGreen *= 1.0f - rain * 0.5f;
    fogBlue *= 1.0f - rain * 0.4f;
  }
  const float thunder = option::thunderGradient(resolved, &world, tickDelta);
  if(thunder > 0.0f) {
    const float dark = 1.0f - thunder * 0.5f;
    fogRed *= dark;
    fogGreen *= dark;
    fogBlue *= dark;
  }
  const auto* living = dynamic_cast<const LivingEntity*>(client->camera);
  if(living != nullptr && living->isInFluid(::net::minecraft::block::material::Material::WATER)) {
    fogRed = resolved.clearWater ? 0.05f : 0.02f;
    fogGreen = resolved.clearWater ? 0.05f : 0.02f;
    fogBlue = resolved.clearWater ? 0.35f : 0.2f;
  } else if(living != nullptr && living->isInFluid(::net::minecraft::block::material::Material::LAVA)) {
    fogRed = 0.6f;
    fogGreen = 0.1f;
    fogBlue = 0.0f;
  } else if(resolved.customFogColor) {
    fogRed = resolved.fogColorRed;
    fogGreen = resolved.fogColorGreen;
    fogBlue = resolved.fogColorBlue;
  }
  gl::clearColor(fogRed, fogGreen, fogBlue, 0.0f);
}
void GameRenderer::applyFog(int mode) {
  if(client == nullptr || client->world == nullptr || client->camera == nullptr) {
    return;
  }
  constexpr int fogModeLinear = 0x2601;
  constexpr int fogModeExp = 0x0800;
  const float color[4] = {fogRed, fogGreen, fogBlue, 1.0f};
  gl::fogfv(gl::fog::Color, color);
  gl::normal3f(0.0f, -1.0f, 0.0f);
  gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
  const option::ResolvedRenderOptions resolved = option::resolve(client->options);
  const auto* living = dynamic_cast<const LivingEntity*>(client->camera);
  if(living != nullptr && living->isInFluid(::net::minecraft::block::material::Material::WATER)) {
    gl::fogi(gl::fog::Mode, fogModeExp);
    gl::fogf(gl::fog::Density, resolved.clearWater ? 0.02f : 0.1f);
  } else if(living != nullptr && living->isInFluid(::net::minecraft::block::material::Material::LAVA)) {
    gl::fogi(gl::fog::Mode, fogModeExp);
    gl::fogf(gl::fog::Density, 2.0f);
  } else if(resolved.customFog && !resolved.customFogLinear) {
    gl::fogi(gl::fog::Mode, fogModeExp);
    gl::fogf(gl::fog::Density, resolved.fogDensity / resolved.renderScale);
  } else {
    gl::fogi(gl::fog::Mode, fogModeLinear);
    if(mode < 0) {
      gl::fogf(gl::fog::Start, 0.0f);
      gl::fogf(gl::fog::End, resolved.renderDistanceBlocks * 0.8f);
    } else {
      gl::fogf(gl::fog::Start,
               resolved.renderDistanceBlocks * (resolved.customFog ? resolved.fogStart : 0.25f));
      gl::fogf(gl::fog::End,
               resolved.renderDistanceBlocks * (resolved.customFog ? resolved.fogEnd : 1.0f));
    }
    if(client->world->dimension != nullptr && client->world->dimension->isNether) {
      gl::fogf(gl::fog::Start, 0.0f);
    }
  }
  const gl::preset::ColorMaterialAmbient colorMaterialCaps;
}
void GameRenderer::renderWorld(float tickDelta, float fov) {
  if(client == nullptr) {
    return;
  }
  int viewport[4]{0, 0, client->displayWidth, client->displayHeight};
  gl::getIntegerv(gl::query::Viewport, viewport);
  const float aspect = viewport[3] != 0 ? static_cast<float>(viewport[2]) / static_cast<float>(viewport[3]) : 1.0f;
  const option::ResolvedRenderOptions resolved = option::resolve(client->options);
  gl::matrixMode(gl::matrix_::Projection);
  gl::loadIdentity();
  if(zoom != 1.0) {
    gl::translatef(static_cast<float>(zoomX), static_cast<float>(-zoomY), 0.0f);
    gl::scaled(zoom, zoom, 1.0);
    gluPerspectiveFov(fov, aspect, 0.05f, resolved.renderDistanceBlocks * 2.0f);
  } else {
    gluPerspectiveFov(fov, aspect, 0.05f, resolved.renderDistanceBlocks * 2.0f);
  }
  gl::matrixMode(gl::matrix_::ModelView);
  gl::loadIdentity();
  if(!frameCamera_.customView) {
    applyDamageTiltEffect(tickDelta);
    if(client->options.bobView) {
      applyViewBobbing(tickDelta);
    }
    if(client->player != nullptr) {
      const float distortion = client->player->lastScreenDistortion +
                               (client->player->screenDistortion - client->player->lastScreenDistortion) * tickDelta;
      if(distortion > 0.0f) {
        float scale = 5.0f / (distortion * distortion + 5.0f) - distortion * 0.04f;
        scale *= scale;
        gl::rotatef((static_cast<float>(ticks) + tickDelta) * 20.0f, 0.0f, 1.0f, 1.0f);
        gl::scalef(1.0f / scale, 1.0f, 1.0f);
        gl::rotatef(-((static_cast<float>(ticks) + tickDelta) * 20.0f), 0.0f, 1.0f, 1.0f);
      }
    }
  }
  applyCameraTransform(tickDelta);
}
void GameRenderer::renderFirstPersonHand(float tickDelta) {
  if(client == nullptr || heldItemRenderer == nullptr || frameCamera_.hideFirstPersonHand) {
    return;
  }
  int viewport[4]{0, 0, client->displayWidth, client->displayHeight};
  gl::getIntegerv(gl::query::Viewport, viewport);
  const option::ResolvedRenderOptions resolved = option::resolve(client->options);
  const float aspect = viewport[3] != 0 ? static_cast<float>(viewport[2]) / static_cast<float>(viewport[3]) : 1.0f;
  gl::matrixMode(gl::matrix_::Projection);
  gl::loadIdentity();
  if(zoom != 1.0) {
    gl::translatef(static_cast<float>(zoomX), static_cast<float>(-zoomY), 0.0f);
    gl::scaled(zoom, zoom, 1.0);
  }
  gluPerspectiveFov(getFov(tickDelta), aspect, 0.05f, resolved.renderDistanceBlocks * 2.0f);
  gl::matrixMode(gl::matrix_::ModelView);
  gl::loadIdentity();
  const gl::preset::FirstPersonDepth handDepth;
  auto* living = dynamic_cast<LivingEntity*>(client->camera);
  // WorldRenderer::renderEntities may have early-returned this frame (entity render
  // cooldown after a world reload) without refreshing the dispatcher, leaving its
  // camera pointing at a deleted entity — e.g. the pre-respawn player. Refresh it
  // here so PlayerEntityRenderer::renderHand never sees a stale pointer.
  entity::EntityRenderDispatcher::instance().setCameraEntity(living);
  {
    const gl::MatrixGuard matrix;
    applyDamageTiltEffect(tickDelta);
    if(client->options.bobView) {
      applyViewBobbing(tickDelta);
    }
    if(living != nullptr) {
      mod::FirstPersonHandRenderEvent event{living, tickDelta, 0, false};
      mod::hooks().publish(event);
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
    applyDamageTiltEffect(tickDelta);
  }
  if(client->options.bobView) {
    applyViewBobbing(tickDelta);
  }
}
void GameRenderer::onFrameUpdate(float tickDelta) {
  if(client == nullptr) {
    return;
  }
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
    } else if(option::resolve(client->options).smoothInput) {
      deltaYaw = yawSmoother.smooth(deltaYaw, 0.15f * scale);
      deltaPitch = pitchSmoother.smooth(deltaPitch, 0.15f * scale);
    }
    client->player->changeLookDirection(deltaYaw, deltaPitch * static_cast<float>(invert));
  }
  if(client->skipGameRender) {
    return;
  }
  int fpsCap = 200;
  if(client->options.fpsLimit == 1) {
    fpsCap = 120;
  } else if(client->options.fpsLimit == 2) {
    fpsCap = 40;
  }
  if(client->world != nullptr) {
    if(client->options.fpsLimit == 0) {
      renderFrame(tickDelta, 0);
    } else {
      renderFrame(tickDelta, lastFrameTime + (1'000'000'000LL / fpsCap));
    }
    throttleAndTimestamp(fpsCap);
    if(!client->options.hideHud || client->currentScreen() != nullptr) {
      const bool chatOpen = dynamic_cast<gui::screen::ChatScreen*>(client->currentScreen()) != nullptr;
      setupHudRender();
      client->inGameHud.render(tickDelta, chatOpen, 0, 0);
    }
  } else {
    gl::viewport(0, 0, client->displayWidth, client->displayHeight);
    gl::clear(gl::attrib::ColorBufferBit | gl::attrib::DepthBufferBit);
    gl::matrixMode(gl::matrix_::Projection);
    gl::loadIdentity();
    gl::matrixMode(gl::matrix_::ModelView);
    gl::loadIdentity();
    setupHudRender();
    throttleAndTimestamp(fpsCap);
  }
  if(client->currentScreen() != nullptr) {
    input::InputSystem& input = input::InputSystem::instance();
#ifdef _WIN32
    input.syncCursorFromOs();
#endif
    const util::UiScale scale = util::uiScale(client->options, client->displayWidth, client->displayHeight);
    const auto [mouseX, mouseY] =
        util::mapScreenMouse(client->displayWidth, client->displayHeight, scale.scaledWidth, scale.scaledHeight,
                             input.mouseX(), input.mouseY());
    gl::pass::beginScreen(scale, client->displayWidth, client->displayHeight);
    mod::ScreenGuiEvent renderGui{client->currentScreen(), false, tickDelta, mouseX, mouseY};
    mod::hooks().publish(renderGui);
    client->currentScreen()->render(mouseX, mouseY, tickDelta);
  }
}
void GameRenderer::throttleAndTimestamp(int fpsCap) {
  if(client->options.fpsLimit == 2) {
    std::int64_t sleepNs = (lastFrameTime + (1'000'000'000LL / fpsCap) - nowNanos()) / 1'000'000LL;
    if(option::resolve(client->options).smoothFps) {
      sleepNs = sleepNs * 3 / 4;
    }
    if(sleepNs < 0) {
      sleepNs += 10;
    }
    if(sleepNs > 0 && sleepNs < 500) {
      sleepMillis(sleepNs);
    }
  }
  lastFrameTime = nowNanos();
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
[[nodiscard]] AtmosphereContext makeAtmosphereContext(net::minecraft::client::Minecraft* client, LivingEntity* camera,
                                                      int atmosphereTicks) {
  return AtmosphereContext{
      .client = client,
      .world = client->world,
      .textureManager = &client->textureManager,
      .camera = client->camera,
      .livingCamera = camera,
      .options = client->options,
      .atmosphereTicks = atmosphereTicks,
  };
}
void bindTerrainTexture(int terrainTextureId) {
  if(terrainTextureId < 0) {
    return;
  }
  gl::activeTexture(gl::tex::Texture0);
  gl::setCap(gl::cap::Texture2D, true);
  gl::bindTexture(gl::cap::Texture2D, static_cast<unsigned int>(terrainTextureId));
}
void drawSolidTerrain(WorldRenderer& worldRenderer, LivingEntity& camera, float tickDelta, int terrainTextureId,
                      bool ambientOcclusion) {
  bindTerrainTexture(terrainTextureId);
  gl::setCap(gl::cap::AlphaTest, true);
  gl::alphaFunc(gl::compare::Greater, 0.1f);
  platform::Lighting::turnOff();
  if(ambientOcclusion) {
    gl::shadeModel(gl::shade::Smooth);
  }
  gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
  worldRenderer.render(camera, 0, static_cast<double>(tickDelta));
  gl::shadeModel(gl::shade::Flat);
}
void drawTranslucentTerrain(WorldRenderer& worldRenderer, LivingEntity& camera, float tickDelta, int terrainTextureId,
                            bool fancyGraphics, bool ambientOcclusion) {
  bindTerrainTexture(terrainTextureId);
  const gl::preset::TranslucentTerrain terrainCaps;
  if(fancyGraphics) {
    if(ambientOcclusion) {
      gl::shadeModel(gl::shade::Smooth);
    }
    gl::colorMask(false, false, false, false);
    worldRenderer.render(camera, 1, static_cast<double>(tickDelta), false);
    gl::colorMask(true, true, true, true);
    worldRenderer.renderLastChunks(1, static_cast<double>(tickDelta));
    gl::shadeModel(gl::shade::Flat);
  } else {
    worldRenderer.render(camera, 1, static_cast<double>(tickDelta));
  }
  gl::depthMask(true);
  gl::setCap(gl::cap::CullFace, true);
  gl::setCap(gl::cap::Blend, false);
  gl::setCap(gl::cap::AlphaTest, true);
  gl::alphaFunc(gl::compare::Greater, 0.1f);
}
void renderBlockOverlay(WorldRenderer& worldRenderer, net::minecraft::client::Minecraft* client, PlayerEntity& player,
                        float tickDelta) {
  gl::setCap(gl::cap::AlphaTest, false);
  const ItemStack hand = selectedItemOrEmpty(&player);
  worldRenderer.renderMiningProgress(&player, *client->crosshairTarget, 0, hand, tickDelta);
  worldRenderer.renderBlockOutline(&player, *client->crosshairTarget, 0, hand, tickDelta);
  gl::setCap(gl::cap::AlphaTest, true);
}
} // namespace
void GameRenderer::renderFrame(float tickDelta, std::int64_t /*timeNs*/) {
  mod::RenderTargetsEvent renderTargetsEvent{tickDelta};
  mod::hooks().publish(renderTargetsEvent);
  if(client != nullptr) {
    renderToCurrentTarget(tickDelta, FrameRenderCamera{}, getFov(tickDelta), client->displayWidth,
                          client->displayHeight, false);
  }
}
void GameRenderer::renderToCurrentTarget(float tickDelta, const FrameRenderCamera& cameraFrame, float fov,
                                         int viewportWidth, int viewportHeight, bool renderCameraEntity) {
  if(client == nullptr) {
    return;
  }
  gl::viewport(0, 0, viewportWidth, viewportHeight);
  gl::setCap(gl::cap::CullFace, true);
  gl::setCap(gl::cap::DepthTest, true);
  gl::depthMask(true);
  gl::setCap(gl::cap::AlphaTest, true);
  gl::alphaFunc(gl::compare::Greater, 0.1f);
  if(client->camera == nullptr) {
    client->camera = client->player;
  }
  updateTargetedEntity(tickDelta);
  auto* camera = dynamic_cast<LivingEntity*>(client->camera);
  WorldRenderer* worldRenderer = client->worldRenderer.get();
  if(camera == nullptr || client->world == nullptr || worldRenderer == nullptr) {
    return;
  }
  frameCamera_ = cameraFrame;
  worldRenderer->setCamera(camera);
  worldRenderer->setRenderCameraEntity(renderCameraEntity);
  if(!frameCamera_.customView) {
    const float rollAmount = prevCameraRollAmount + (cameraRollAmount - prevCameraRollAmount) * tickDelta;
    mod::CameraSetupEvent cameraEvent{};
    populateCameraSetupDefaults(cameraEvent, *camera, tickDelta, rollAmount);
    mod::hooks().publish(cameraEvent);
    frameCamera_ = frameCameraFromSetup(cameraEvent);
  }
  RenderCameraState::instance().setFrame(frameCamera_);
  worldRenderer->setFrameRenderCamera(frameCamera_.x, frameCamera_.y, frameCamera_.z);
  client->world->setChunkCacheCenterFromBlockPos(MathHelper::floor(frameCamera_.x), MathHelper::floor(frameCamera_.z));
  const option::ResolvedRenderOptions resolvedOptions = option::resolve(client->options);
  const bool ambientOcclusion = client->options.ao;
  const bool fancyGraphics = client->options.fancyGraphics;
  const int terrainTextureId = client->textureManager.getTextureId("/terrain.png");
  const AtmosphereContext atmosphereCtx = makeAtmosphereContext(client, camera, ticks);
  updateSkyAndFogColors(tickDelta);
  gl::clear(gl::attrib::ColorBufferBit | gl::attrib::DepthBufferBit);
  renderWorld(tickDelta, fov);
  if(client->options.frustumCulling) {
    Frustum::getInstance().compute();
  }
  if(resolvedOptions.viewDistanceSetting < 2 && resolvedOptions.renderSky) {
    applyFog(-1);
    if(client->world->dimension != nullptr && !client->world->dimension->isNether) {
      atmosphere::renderSkyDome(atmosphereCtx, tickDelta);
    }
    gl::setCap(gl::cap::AlphaTest, true);
    gl::alphaFunc(gl::compare::Greater, 0.1f);
  }
  gl::setCap(gl::cap::Fog, true);
  applyFog(1);
  if(ambientOcclusion) {
    gl::shadeModel(gl::shade::Smooth);
  }
  FrustumCuller frustumCuller;
  FrustumCuller* activeCuller = nullptr;
  if(client->options.frustumCulling) {
    frustumCuller.prepare(frameCamera_.x, frameCamera_.y, frameCamera_.z);
    activeCuller = &frustumCuller;
  }
  worldRenderer->cullChunks(activeCuller, tickDelta);
  worldRenderer->compileChunks(*camera, false);
  applyFog(0);
  drawSolidTerrain(*worldRenderer, *camera, tickDelta, terrainTextureId, ambientOcclusion);
  platform::Lighting::turnOn();
  const Vec3d frameCameraPos{frameCamera_.x, frameCamera_.y, frameCamera_.z};
  worldRenderer->renderEntities(frameCameraPos, activeCuller, tickDelta);
  gl::setCap(gl::cap::AlphaTest, true);
  gl::alphaFunc(gl::compare::Greater, 0.1f);
  client->particleManager.renderLit(camera, tickDelta);
  platform::Lighting::turnOff();
  applyFog(0);
  gl::setCap(gl::cap::Blend, true);
  gl::blendFunc(gl::blend::SrcAlpha, gl::blend::OneMinusSrcAlpha);
  client->particleManager.render(camera, tickDelta);
  if(client->crosshairTarget.has_value()) {
    if(auto* player = dynamic_cast<PlayerEntity*>(camera)) {
      if(camera->isInFluid(::net::minecraft::block::material::Material::WATER)) {
        renderBlockOverlay(*worldRenderer, client, *player, tickDelta);
      }
    }
  }
  applyFog(0);
  drawTranslucentTerrain(*worldRenderer, *camera, tickDelta, terrainTextureId, fancyGraphics, ambientOcclusion);
  if(client->crosshairTarget.has_value() && zoom == 1.0 &&
     !camera->isInFluid(::net::minecraft::block::material::Material::WATER)) {
    if(auto* player = dynamic_cast<PlayerEntity*>(camera)) {
      renderBlockOverlay(*worldRenderer, client, *player, tickDelta);
    }
  }
  precipitationRenderer.renderPrecipitation(atmosphereCtx, tickDelta);
  {
    const gl::preset::FogOff preCloudFog;
    applyFog(0);
  }
  mod::WorldRenderEvent cloudEvent{
      atmosphereCtx.world,
      atmosphereCtx.camera,
      tickDelta,
      mod::WorldRenderStage::Clouds,
      mod::RenderHookMoment::Before,
  };
  mod::hooks().publish(cloudEvent);
  if(!cloudEvent.cancelVanilla && resolvedOptions.renderClouds) {
    cloudRenderer.renderClouds(atmosphereCtx, tickDelta);
    cloudEvent.vanillaStageRan = true;
  }
  cloudEvent.moment = mod::RenderHookMoment::After;
  mod::hooks().publish(cloudEvent);
  gl::setCap(gl::cap::Fog, false);
  applyFog(1);
  gl::setCap(gl::cap::CullFace, true);
  gl::setCap(gl::cap::Blend, false);
  gl::setCap(gl::cap::AlphaTest, true);
  gl::alphaFunc(gl::compare::Greater, 0.1f);
  gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
  if(zoom == 1.0 && !renderCameraEntity) {
    gl::clear(gl::attrib::DepthBufferBit);
    renderFirstPersonHand(tickDelta);
  }
}
void GameRenderer::renderRain() {
  if(client == nullptr || client->world == nullptr || client->camera == nullptr) {
    return;
  }
  float rain = worldRainGradient(client, client->world, 1.0f);
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
    if(&block->material == &::net::minecraft::block::material::Material::LAVA) {
      world->addParticle("smoke", static_cast<float>(px) + rx, py, static_cast<float>(pz) + rz, 0.0, 0.0, 0.0);
      continue;
    }
    world->addParticle("rainsplash", static_cast<float>(px) + rx, py, static_cast<float>(pz) + rz, 0.0, 0.0, 0.0);
  }
}
void GameRenderer::setupHudRender() {
  if(client == nullptr) {
    return;
  }
  gl::pass::beginHud(util::uiScale(client->options, client->displayWidth, client->displayHeight));
}
} // namespace net::minecraft::client::render
