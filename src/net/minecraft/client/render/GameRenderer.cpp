#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackManager.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/debug/RenderProfiler.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/EnginePipeline.hpp"
#include "net/minecraft/client/gui/screen/ChatScreen.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/atmosphere/AtmosphereContext.hpp"
#include "net/minecraft/client/render/atmosphere/SkyDome.hpp"
#include "net/minecraft/client/render/culling/Frustum.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include <charconv>
#include <filesystem>
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/world/WorldRenderer.hpp"
#include "net/minecraft/client/util/UiScale.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/mod/model/ModModels.hpp"
#include "net/minecraft/util/hit/HitResult.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/MatrixStacks.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
#ifdef _WIN32
#include "net/minecraft/client/util/DisplayManager.hpp"
#endif
#ifdef MINECRAFT_GL_REAL
#include <GL/glu.h>
#include <algorithm>
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
[[nodiscard]] bool hasActiveGlContext() {
#ifdef _WIN32
 return wglGetCurrentContext() != nullptr;
#else
 return gl::GLCore::activeTexture != nullptr;
#endif
}
void gluPerspectiveFov(float fovyDeg, float aspect, float zNear, float zFar) {
 // Core profile: build the projection into the CPU MatrixStack (the ubershader reads
 // it as a uniform). Callers load identity on the projection stack first, so a
 // multiply is equivalent to a load.
 net::minecraft::util::math::Matrix4f m;
 m.perspective(fovyDeg, aspect, zNear, zFar);
 net::minecraft::util::math::g_projection.multiply(m);
}
// GL_FOG used to be mirrored into a CPU PipelineState (now removed along with
// GlState.hpp/GlPresets.hpp); it is reproduced here as persistent fog state that
// is pushed to the ubershader via engine_pipeline::setFog on every mutation, the
// same way glFogi/glFogf/glFogfv + the GL_FOG cap used to behave.
gl::engine_pipeline::FogUniforms gFogUniforms{};
constexpr int kGlFogLinear = 0x2601;
constexpr int kGlFogExp = 0x0800;
constexpr int kGlFogExp2 = 0x0801;
constexpr int kGlFuncAlways = 0x0207;
[[nodiscard]] int translateGlFogMode(int glMode) {
 switch(glMode) {
 case kGlFogLinear: return 1;
 case kGlFogExp: return 2;
 case kGlFogExp2: return 3;
 default: return 0;
 }
}
void setFogEnabled(bool enabled) {
 gFogUniforms.enabled = enabled;
 gl::engine_pipeline::setFog(gFogUniforms);
 RenderSystem::hintFogEnabled(enabled);
}
void setFogColor(const float color[4]) {
 gFogUniforms.color[0] = color[0];
 gFogUniforms.color[1] = color[1];
 gFogUniforms.color[2] = color[2];
 gFogUniforms.color[3] = color[3];
 gl::engine_pipeline::setFog(gFogUniforms);
}
void setFogModeGl(int glMode) {
 gFogUniforms.mode = translateGlFogMode(glMode);
 gl::engine_pipeline::setFog(gFogUniforms);
}
void setFogDensity(float density) {
 gFogUniforms.density = density;
 gl::engine_pipeline::setFog(gFogUniforms);
}
void setFogStart(float start) {
 gFogUniforms.start = start;
 gl::engine_pipeline::setFog(gFogUniforms);
}
void setFogEnd(float end) {
 gFogUniforms.end = end;
 gl::engine_pipeline::setFog(gFogUniforms);
}
// Local RAII scopes reimplemented against RenderSystem's getShadow()/
// setShadow() (matrix::Guard, TranslucentTerrain, FirstPersonDepth, ScreenFogOff,
// BoundTextureScope from the removed GlDraw.hpp/GlPresets.hpp).
struct MatrixScope {
 MatrixScope() { RenderSystem::pushMatrix(); }
 ~MatrixScope() { RenderSystem::popMatrix(); }
 MatrixScope(const MatrixScope&) = delete;
 MatrixScope& operator=(const MatrixScope&) = delete;
};
struct FirstPersonDepthScope {
 RenderSystem::StateShadow saved_ = RenderSystem::getShadow();
 FirstPersonDepthScope() {
  RenderSystem::enableDepthTest();
  RenderSystem::depthMask(true);
 }
 ~FirstPersonDepthScope() { RenderSystem::setShadow(saved_); }
 FirstPersonDepthScope(const FirstPersonDepthScope&) = delete;
 FirstPersonDepthScope& operator=(const FirstPersonDepthScope&) = delete;
};
struct TranslucentTerrainScope {
 RenderSystem::StateShadow saved_ = RenderSystem::getShadow();
 TranslucentTerrainScope() {
  RenderSystem::blendAlpha();
  RenderSystem::disableCull();
 }
 ~TranslucentTerrainScope() { RenderSystem::setShadow(saved_); }
 TranslucentTerrainScope(const TranslucentTerrainScope&) = delete;
 TranslucentTerrainScope& operator=(const TranslucentTerrainScope&) = delete;
};
struct FogOffScope {
 bool saved_ = gFogUniforms.enabled;
 FogOffScope() { setFogEnabled(false); }
 ~FogOffScope() { setFogEnabled(saved_); }
 FogOffScope(const FogOffScope&) = delete;
 FogOffScope& operator=(const FogOffScope&) = delete;
};
struct BoundTextureScope {
 int saved_ = 0;
 BoundTextureScope() { RenderSystem::getIntegerv(gl::tex::Binding2D, &saved_); }
 ~BoundTextureScope() { RenderSystem::bindTexture(saved_); }
 BoundTextureScope(const BoundTextureScope&) = delete;
 BoundTextureScope& operator=(const BoundTextureScope&) = delete;
};
void drawFullscreenTexturedQuad(int width, int height) {
 int savedViewport[4]{0, 0, width, height};
 RenderSystem::getIntegerv(gl::query::Viewport, savedViewport);
 const RenderSystem::StateShadow saved = RenderSystem::getShadow();
 RenderSystem::disableDepthTest();
 RenderSystem::disableCull();
 RenderSystem::disableBlend();
 RenderSystem::viewport(0, 0, width, height);
 RenderSystem::depthMask(false);
 RenderSystem::activeTexture(gl::tex::Texture0);
 gl::engine_pipeline::blitFullscreen();
 RenderSystem::setShadow(saved);
 RenderSystem::viewport(savedViewport[0], savedViewport[1], savedViewport[2], savedViewport[3]);
}
void applyOrthoProjection(const util::UiScale& scale) {
 RenderSystem::matrixMode(gl::matrix_::Projection);
 RenderSystem::loadIdentity();
 RenderSystem::ortho(0.0, scale.rawWidth, scale.rawHeight, 0.0, 1000.0, 3000.0);
 RenderSystem::matrixMode(gl::matrix_::ModelView);
 RenderSystem::loadIdentity();
 RenderSystem::translate(0.0f, 0.0f, -2000.0f);
}
void applyHudEnables() {
 RenderSystem::disableCull();
 RenderSystem::enableTexture();
 RenderSystem::alphaTest(0.1f);
 RenderSystem::color4f(1.0f, 1.0f, 1.0f, 1.0f);
}
void applyScreenEnables() {
 RenderSystem::disableCull();
 render::RenderSystem::disableLighting();
 setFogEnabled(false);
 RenderSystem::enableTexture();
 RenderSystem::alphaTest(0.1f);
 RenderSystem::color4f(1.0f, 1.0f, 1.0f, 1.0f);
}
void beginHud(const util::UiScale& scale, int viewportW, int viewportH) {
 RenderSystem::viewport(0, 0, viewportW, viewportH);
 RenderSystem::clear(gl::attrib::DepthBufferBit);
 applyOrthoProjection(scale);
 applyHudEnables();
}
void beginScreen(const util::UiScale& scale, int viewportW, int viewportH) {
 RenderSystem::viewport(0, 0, viewportW, viewportH);
 applyScreenEnables();
 RenderSystem::clear(gl::attrib::DepthBufferBit);
 applyOrthoProjection(scale);
 RenderSystem::clear(gl::attrib::DepthBufferBit);
}
[[nodiscard]] float effectiveFarPlane(const option::ResolvedRenderOptions& resolved) {
 float farPlane = resolved.renderDistanceBlocks * 2.0f;
 return farPlane;
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
    : client(clientIn),
      heldItemRenderer(std::make_unique<item::HeldItemRenderer>(clientIn)),
      lastInactiveTime(nowMillis()),
      shaderPacks_(clientIn != nullptr
                       ? std::make_unique<shaderpack::ShaderPackManager>(
                             net::minecraft::client::Minecraft::getRunDirectory(),
                             &clientIn->options)
                       : nullptr) {
}
GameRenderer::~GameRenderer() {
 if(lightmapTexture_ != 0) {
  RenderSystem::deleteTexture(lightmapTexture_);
  lightmapTexture_ = 0;
 }
}
void GameRenderer::updateLightmap(float tickDelta) {
 if(client == nullptr || client->world == nullptr || client->world->dimension == nullptr) {
  return;
 }
 if(lightmapTexture_ == 0) {
  lightmapTexture_ = RenderSystem::genTexture();
  RenderSystem::activeTexture(0x84C1); // GL_TEXTURE1
  RenderSystem::bindTexture(lightmapTexture_);
  ::glTexParameteri(0x0DE1, 0x2801, 0x2601); // GL_TEXTURE_MIN_FILTER, GL_LINEAR
  ::glTexParameteri(0x0DE1, 0x2800, 0x2601); // GL_TEXTURE_MAG_FILTER, GL_LINEAR
  ::glTexParameteri(0x0DE1, 0x2802, 0x812F); // GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE
  ::glTexParameteri(0x0DE1, 0x2803, 0x812F); // GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE
  ::glTexImage2D(0x0DE1, 0, 0x8058, 16, 16, 0, 0x1908, 0x1401, nullptr);
  RenderSystem::activeTexture(0x84C0); // GL_TEXTURE0
  lightmapColorsValid_ = false;
 }
 float skyIntensity = client->world->calculateSkyLightIntensity(tickDelta);
 float skyFactor = skyIntensity * 0.95f + 0.05f;
 const float* luminanceTable = client->world->dimension->lightLevelToLuminance.data();
 std::uint32_t colors[256];
 for(int sky = 0; sky < 16; ++sky) {
  float skyLuminance = luminanceTable[sky] * skyFactor;
  for(int block = 0; block < 16; ++block) {
   float blockLuminance = luminanceTable[block];
   float r = blockLuminance * 1.2f + skyLuminance * 0.9f;
   float g = blockLuminance * 1.0f + skyLuminance * 0.9f;
   float b = blockLuminance * 0.8f + skyLuminance * 1.0f;
   r = r * 0.5f + std::sqrt(r) * 0.5f;
   g = g * 0.5f + std::sqrt(g) * 0.5f;
   b = b * 0.5f + std::sqrt(b) * 0.5f;
   r = std::clamp(r, 0.0f, 1.0f);
   g = std::clamp(g, 0.0f, 1.0f);
   b = std::clamp(b, 0.0f, 1.0f);
   std::uint8_t ri = static_cast<std::uint8_t>(r * 255.0f);
   std::uint8_t gi = static_cast<std::uint8_t>(g * 255.0f);
   std::uint8_t bi = static_cast<std::uint8_t>(b * 255.0f);
   colors[sky * 16 + block] = ri | (gi << 8) | (bi << 16) | (255U << 24);
  }
 }
 if(lightmapColorsValid_ && std::equal(std::begin(colors), std::end(colors), lastLightmapColors_)) {
  return;
 }
 std::copy(std::begin(colors), std::end(colors), lastLightmapColors_);
 lightmapColorsValid_ = true;
 RenderSystem::activeTexture(0x84C1); // GL_TEXTURE1
 RenderSystem::bindTexture(lightmapTexture_);
 ::glTexSubImage2D(0x0DE1, 0, 0, 0, 16, 16, 0x1908, 0x1401, colors);
 RenderSystem::activeTexture(0x84C0); // GL_TEXTURE0
}
float GameRenderer::farPlaneBlocks() const {
 if(client == nullptr) {
  return 192.0f;
 }
 return effectiveFarPlane(option::resolve(client->options));
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
 net::minecraft::mod::model::ModelRaycastHit modelHit;
 if(net::minecraft::mod::model::raycastModelInstances(
        eyePos.x, eyePos.y, eyePos.z, look.x, look.y, look.z, reach, modelHit)) {
  const double limit = targetedEntity != nullptr ? (closest == 0.0 ? 0.0 : closest) : reach;
  if(modelHit.distance < limit) {
   // A "prefix:entityId" tag redirects targeting to the entity the model
   // stands in for. Guard both the cast (client->world may not be a
   // ClientWorld) and the parse (the suffix may not be a number).
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
 if(living->isInFluid(::net::minecraft::block::material::Material::WATER)) {
  fov = 60.0f;
 }
 if(living->health <= 0) {
  const float death = static_cast<float>(living->deathTime) + tickDelta;
  fov /= (1.0f - 500.0f / (death + 500.0f)) * 2.0f + 1.0f;
 }
 fov = option::adjustFieldOfView(fov, option::resolve(client->options));
 mod::FovEvent event{living, tickDelta, fov};
  net::minecraft::mod::runtime::luaHookFov(event);
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
  RenderSystem::rotate(40.0f - 8000.0f / (death + 200.0f), 0.0f, 0.0f, 1.0f);
 }
 if(hurt < 0.0f) {
  return;
 }
 hurt /= static_cast<float>(living->damagedTime);
 hurt = MathHelper::sin(hurt * hurt * hurt * hurt * kPiF);
 const float swing = living->damagedSwingDir;
 RenderSystem::rotate(-swing, 0.0f, 1.0f, 0.0f);
 RenderSystem::rotate(-hurt * 14.0f, 0.0f, 0.0f, 1.0f);
 RenderSystem::rotate(swing, 0.0f, 1.0f, 0.0f);
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
 RenderSystem::translate(
     MathHelper::sin(phase * kPiF) * stepBob * 0.5f, -std::abs(MathHelper::cos(phase * kPiF) * stepBob), 0.0f);
 RenderSystem::rotate(MathHelper::sin(phase * kPiF) * stepBob * 3.0f, 0.0f, 0.0f, 1.0f);
 RenderSystem::rotate(std::abs(MathHelper::cos(phase * kPiF - 0.2f) * stepBob) * 5.0f, 1.0f, 0.0f, 0.0f);
 RenderSystem::rotate(tiltBob, 1.0f, 0.0f, 0.0f);
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
  RenderSystem::rotate(frameCamera_.roll, 0.0f, 0.0f, 1.0f);
  RenderSystem::rotate(frameCamera_.pitch, 1.0f, 0.0f, 0.0f);
  RenderSystem::rotate(frameCamera_.yaw + 180.0f, 0.0f, 1.0f, 0.0f);
  return;
 }
 float eyeOffset = living->standingEyeHeight - 1.62f;
 double interpX = living->lastTickX + (living->x - living->lastTickX) * static_cast<double>(tickDelta);
 double interpY =
     living->lastTickY + (living->y - living->lastTickY) * static_cast<double>(tickDelta) - static_cast<double>(eyeOffset);
 double interpZ = living->lastTickZ + (living->z - living->lastTickZ) * static_cast<double>(tickDelta);
 RenderSystem::rotate(prevCameraRollAmount + (cameraRollAmount - prevCameraRollAmount) * tickDelta, 0.0f, 0.0f, 1.0f);
 if(living->isSleeping()) {
  eyeOffset += 1.0f;
  RenderSystem::translate(0.0f, 0.3f, 0.0f);
  if(!client->options.debugCamera && client->world != nullptr) {
   const int blockId = client->world->getBlockId(
       MathHelper::floor(living->x), MathHelper::floor(living->y), MathHelper::floor(living->z));
   if(blockId == kBedBlockId) {
    const int meta = client->world->getBlockMeta(
        MathHelper::floor(living->x), MathHelper::floor(living->y), MathHelper::floor(living->z));
    const int facing = static_cast<int>(meta) & 3;
    RenderSystem::rotate(static_cast<float>(facing) * 90.0f, 0.0f, 1.0f, 0.0f);
   }
   RenderSystem::rotate(living->prevYaw + (living->yaw - living->prevYaw) * tickDelta + 180.0f, 0.0f, -1.0f, 0.0f);
   RenderSystem::rotate(living->prevPitch + (living->pitch - living->prevPitch) * tickDelta, -1.0f, 0.0f, 0.0f);
  }
 } else if(client->options.thirdPerson) {
  double camDist =
      prevThirdPersonDistance + (thirdPersonDistance - prevThirdPersonDistance) * static_cast<double>(tickDelta);
  if(client->options.debugCamera) {
   const float dbgYaw = prevThirdPersonYaw + (thirdPersonYaw - prevThirdPersonYaw) * tickDelta;
   const float dbgPitch = prevThirdPersonPitch + (thirdPersonPitch - prevThirdPersonPitch) * tickDelta;
   RenderSystem::translate(0.0f, 0.0f, static_cast<float>(-camDist));
   RenderSystem::rotate(dbgPitch, 1.0f, 0.0f, 0.0f);
   RenderSystem::rotate(dbgYaw, 0.0f, 1.0f, 0.0f);
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
   RenderSystem::rotate(living->pitch - basePitch, 1.0f, 0.0f, 0.0f);
   RenderSystem::rotate(living->yaw - baseYaw, 0.0f, 1.0f, 0.0f);
   RenderSystem::translate(0.0f, 0.0f, static_cast<float>(-camDist));
   RenderSystem::rotate(baseYaw - living->yaw, 0.0f, 1.0f, 0.0f);
   RenderSystem::rotate(basePitch - living->pitch, 1.0f, 0.0f, 0.0f);
  }
 } else {
  RenderSystem::translate(0.0f, 0.0f, -0.1f);
 }
 if(!client->options.debugCamera) {
  RenderSystem::rotate(living->prevPitch + (living->pitch - living->prevPitch) * tickDelta, 1.0f, 0.0f, 0.0f);
  RenderSystem::rotate(living->prevYaw + (living->yaw - living->prevYaw) * tickDelta + 180.0f, 0.0f, 1.0f, 0.0f);
 }
 RenderSystem::translate(0.0f, eyeOffset, 0.0f);
}
void GameRenderer::updateSkyAndFogColors(float tickDelta) {
 if(client == nullptr || client->world == nullptr || client->camera == nullptr) {
  return;
 }
 World& world = *client->world;
 const option::ResolvedRenderOptions resolved = option::resolve(client->options);
  fogSettings_ = mod::FogSettingsEvent{&world, client->camera};
  net::minecraft::mod::runtime::luaHookFogSettings(fogSettings_);
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
 } else if(fogSettings_.enabled && fogSettings_.customColor) {
  fogRed = fogSettings_.red;
  fogGreen = fogSettings_.green;
  fogBlue = fogSettings_.blue;
 }
 RenderSystem::clearColor(fogRed, fogGreen, fogBlue, 0.0f);
}
void GameRenderer::applyFog(int mode) {
 if(client == nullptr || client->world == nullptr || client->camera == nullptr) {
  return;
 }
 constexpr int fogModeLinear = 0x2601;
 constexpr int fogModeExp = 0x0800;
 const float color[4] = {fogRed, fogGreen, fogBlue, 1.0f};
 setFogColor(color);
 RenderSystem::color4f(1.0f, 1.0f, 1.0f, 1.0f);
 const option::ResolvedRenderOptions resolved = option::resolve(client->options);
 const auto* living = dynamic_cast<const LivingEntity*>(client->camera);
 if(living != nullptr && living->isInFluid(::net::minecraft::block::material::Material::WATER)) {
  const float density = resolved.clearWater ? 0.02f : 0.1f;
  setFogModeGl(fogModeExp);
  setFogDensity(density);
  if(mode >= 0) {
   lastFogEnd_ = 3.0f / density;
  }
 } else if(living != nullptr && living->isInFluid(::net::minecraft::block::material::Material::LAVA)) {
  setFogModeGl(fogModeExp);
  setFogDensity(2.0f);
  if(mode >= 0) {
   lastFogEnd_ = 1.5f;
  }
 } else if(fogSettings_.enabled && fogSettings_.exponential) {
  const float density = fogSettings_.density / resolved.renderScale;
  setFogModeGl(fogModeExp);
  setFogDensity(density);
  if(mode >= 0) {
   lastFogEnd_ = density > 0.0f ? 3.0f / density : resolved.renderDistanceBlocks;
  }
 } else {
  setFogModeGl(fogModeLinear);
  constexpr bool terrainFog = false;
  const float terrainEdge = static_cast<float>(resolved.chunkRadius) * 16.0f - 8.0f;
  const float fogRange = std::min(resolved.renderDistanceBlocks, terrainEdge);
  if(mode < 0) {
   setFogStart(0.0f);
   setFogEnd(fogRange * 0.8f);
  } else if(terrainFog) {
   const float fogEnd = fogRange * (fogSettings_.enabled ? fogSettings_.end : 1.0f);
   const float fogStart = std::min(fogRange * (fogSettings_.enabled ? fogSettings_.start : 0.45f), fogEnd * 0.85f);
   setFogStart(fogStart);
   setFogEnd(fogEnd);
   if(mode >= 0) {
    lastFogEnd_ = fogEnd;
   }
  } else {
   // The chunk grid diameter is capped (see ResolvedRenderOptions), so at
   // Far view distance terrain stops well short of renderDistanceBlocks.
   // Fog must be fully opaque by the loaded-terrain edge or the grid edge
   // pops into view.
   const float fogEnd = fogRange * (fogSettings_.enabled ? fogSettings_.end : 1.0f);
   const float fogStart = std::min(fogRange * (fogSettings_.enabled ? fogSettings_.start : 0.25f), fogEnd * 0.75f);
   setFogStart(fogStart);
   setFogEnd(fogEnd);
   if(mode >= 0) {
    lastFogEnd_ = fogEnd;
   }
  }
  if(client->world->dimension != nullptr && client->world->dimension->isNether) {
   setFogStart(0.0f);
  }
 }
}
void GameRenderer::renderWorld(float tickDelta, float fov) {
 if(client == nullptr) {
  return;
 }
 int viewport[4]{0, 0, client->displayWidth, client->displayHeight};
 if(!RenderSystem::getCachedViewport(viewport)) {
  RenderSystem::getIntegerv(gl::query::Viewport, viewport);
 }
 const float aspect = viewport[3] != 0 ? static_cast<float>(viewport[2]) / static_cast<float>(viewport[3]) : 1.0f;
 const option::ResolvedRenderOptions resolved = option::resolve(client->options);
 RenderSystem::matrixMode(gl::matrix_::Projection);
 RenderSystem::loadIdentity();
 const float nearPlane = std::max(0.001f, frameCamera_.perspectiveNear);
 const float farPlane =
     frameCamera_.perspectiveFar > nearPlane ? frameCamera_.perspectiveFar : effectiveFarPlane(resolved);
 if(frameCamera_.orthographic) {
  RenderSystem::ortho(-static_cast<double>(frameCamera_.orthoHalfWidth),
                      static_cast<double>(frameCamera_.orthoHalfWidth),
                      -static_cast<double>(frameCamera_.orthoHalfHeight),
                      static_cast<double>(frameCamera_.orthoHalfHeight),
                      static_cast<double>(frameCamera_.orthoNear),
                      static_cast<double>(frameCamera_.orthoFar));
 } else {
  if(zoom != 1.0) {
   RenderSystem::translate(static_cast<float>(zoomX), static_cast<float>(-zoomY), 0.0f);
   RenderSystem::scale(static_cast<float>(zoom), static_cast<float>(zoom), 1.0f);
  }
  gluPerspectiveFov(fov, aspect, nearPlane, farPlane);
 }
 RenderSystem::matrixMode(gl::matrix_::ModelView);
 RenderSystem::loadIdentity();
 if(!frameCamera_.customView) {
  applyDamageTiltEffect(tickDelta);
  if(client->options.bobView) {
   applyViewBobbing(tickDelta);
  }
  if(client->player != nullptr) {
   const float distortion =
       client->player->lastScreenDistortion +
       (client->player->screenDistortion - client->player->lastScreenDistortion) * tickDelta;
   if(distortion > 0.0f) {
    float scale = 5.0f / (distortion * distortion + 5.0f) - distortion * 0.04f;
    scale *= scale;
    RenderSystem::rotate((static_cast<float>(ticks) + tickDelta) * 20.0f, 0.0f, 1.0f, 1.0f);
    RenderSystem::scale(1.0f / scale, 1.0f, 1.0f);
    RenderSystem::rotate(-((static_cast<float>(ticks) + tickDelta) * 20.0f), 0.0f, 1.0f, 1.0f);
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
 if(!RenderSystem::getCachedViewport(viewport)) {
  RenderSystem::getIntegerv(gl::query::Viewport, viewport);
 }
 const option::ResolvedRenderOptions resolved = option::resolve(client->options);
 const float aspect = viewport[3] != 0 ? static_cast<float>(viewport[2]) / static_cast<float>(viewport[3]) : 1.0f;
 RenderSystem::matrixMode(gl::matrix_::Projection);
 RenderSystem::loadIdentity();
 if(zoom != 1.0) {
  RenderSystem::translate(static_cast<float>(zoomX), static_cast<float>(-zoomY), 0.0f);
  RenderSystem::scale(static_cast<float>(zoom), static_cast<float>(zoom), 1.0f);
 }
 gluPerspectiveFov(getFov(tickDelta), aspect, 0.05f, resolved.renderDistanceBlocks * 2.0f);
 RenderSystem::matrixMode(gl::matrix_::ModelView);
 RenderSystem::loadIdentity();
 const FirstPersonDepthScope handDepth;
 auto* living = dynamic_cast<LivingEntity*>(client->camera);
 // WorldRenderer::renderEntities may have early-returned this frame (entity render
 // cooldown after a world reload) without refreshing the dispatcher, leaving its
 // camera pointing at a deleted entity — e.g. the pre-respawn player. Refresh it
 // here so PlayerEntityRenderer::renderHand never sees a stale pointer.
 entity::EntityRenderDispatcher::instance().setCameraEntity(living);
 {
  const MatrixScope matrix;
  applyDamageTiltEffect(tickDelta);
  if(client->options.bobView) {
   applyViewBobbing(tickDelta);
  }
  if(living != nullptr) {
    mod::FirstPersonHandRenderEvent event{living, tickDelta, 0, false};
    net::minecraft::mod::runtime::luaHookFirstPersonHand(event);
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
  const bool captured = beginSceneCapture();
  if(client->options.fpsLimit == 0) {
   renderFrame(tickDelta, 0);
  } else {
   renderFrame(tickDelta, lastFrameTime + (1'000'000'000LL / fpsCap));
  }
  if(captured) {
   resolveSceneCapture(tickDelta);
  }
  throttleAndTimestamp(fpsCap);
  if(!client->options.hideHud || client->currentScreen() != nullptr) {
   const bool chatOpen = dynamic_cast<gui::screen::ChatScreen*>(client->currentScreen()) != nullptr;
   setupHudRender();
   client->inGameHud.render(tickDelta, chatOpen, 0, 0);
  }
 } else {
  RenderSystem::viewport(0, 0, client->displayWidth, client->displayHeight);
  RenderSystem::clear(gl::attrib::ColorBufferBit | gl::attrib::DepthBufferBit);
  RenderSystem::matrixMode(gl::matrix_::Projection);
  RenderSystem::loadIdentity();
  RenderSystem::matrixMode(gl::matrix_::ModelView);
  RenderSystem::loadIdentity();
  setupHudRender();
  throttleAndTimestamp(fpsCap);
 }
 if(client->currentScreen() != nullptr) {
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
  beginScreen(scale, client->displayWidth, client->displayHeight);
   client->currentScreen()->render(mouseX, mouseY, tickDelta);
 }
}
bool GameRenderer::beginSceneCapture() {
 if(client == nullptr) {
  return false;
 }
 const bool wanted = shaderPacks_ != nullptr && shaderPacks_->active() && shaderPacks_->activeHasPostProcess();
 if(!wanted) {
  if(sceneFramebuffer_.isValid()) {
   sceneFramebuffer_.destroy();
  }
  sceneFramebufferAttempted_ = false;
  return false;
 }
 const int width = std::max(1, client->displayWidth);
 const int height = std::max(1, client->displayHeight);
 if(!sceneFramebuffer_.ensure(width, height, shaderPacks_->sceneColorFormat())) {
  if(!sceneFramebufferAttempted_) {
   sceneFramebufferAttempted_ = true;
   ClientLog::LOGGER.log(LogLevel::Warning,
                         "[shader] scene capture target unavailable — shaderpack post-processing disabled");
  }
  return false;
 }
 sceneFramebufferAttempted_ = false;
 sceneFramebuffer_.begin();
 RenderSystem::clear(gl::attrib::ColorBufferBit | gl::attrib::DepthBufferBit);
 return true;
}
void GameRenderer::resolveSceneCapture(float tickDelta) {
 sceneFramebuffer_.end();
 if(client == nullptr) {
  return;
 }
 const int width = std::max(1, client->displayWidth);
 const int height = std::max(1, client->displayHeight);
 const option::ResolvedRenderOptions resolved = option::resolve(client->options);
 const float farPlane =
     frameCamera_.perspectiveFar > 0.0f ? frameCamera_.perspectiveFar : effectiveFarPlane(resolved);
 const float worldTime = static_cast<float>(ticks) + tickDelta;
 bool posted = false;
 if(shaderPacks_ != nullptr && shaderPacks_->activeHasPostProcess()) {
  posted = shaderPacks_->renderPostProcess(static_cast<int>(sceneFramebuffer_.colorTexture()),
                                           static_cast<int>(sceneFramebuffer_.depthTexture()), width, height,
                                           tickDelta, frameCamera_, farPlane, worldTime, client->world, lastFogEnd_);
 }
 if(!posted) {
  sceneFramebuffer_.blitToScreen(width, height);
 }
}
void GameRenderer::throttleAndTimestamp(int fpsCap) {
 const std::int64_t now = nowNanos();
 if(lastFrameTime == 0) {
  lastFrameTime = now;
  return;
 }
 if(client->options.fpsLimit != 0) {
  const std::int64_t targetNs = 1'000'000'000LL / fpsCap;
  const std::int64_t elapsedNs = now - lastFrameTime;
  const std::int64_t sleepNs = targetNs - elapsedNs;
  if(sleepNs > 0) {
   std::int64_t sleepMs = sleepNs / 1'000'000LL;
   if(option::resolve(client->options).smoothFps) {
    sleepMs = sleepMs * 3 / 4;
   }
   if(sleepMs > 0 && sleepMs < 500) {
    sleepMillis(sleepMs);
   }
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
[[nodiscard]] AtmosphereContext makeAtmosphereContext(net::minecraft::client::Minecraft* client,
                                                      LivingEntity* camera,
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
void applyEntityLightingRig(const FrameRenderCamera& camera,
                            gl::engine_pipeline::WorldLightUniforms rig) {
 constexpr float kLen = 1.2369317f;
 constexpr float kKeyX = 0.2f / kLen;
 constexpr float kKeyY = 1.0f / kLen;
 constexpr float kKeyZ = -0.7f / kLen;
 rig.sunDirView[0] = kKeyX * camera.viewRightX + kKeyY * camera.viewRightY + kKeyZ * camera.viewRightZ;
 rig.sunDirView[1] = kKeyX * camera.viewUpX + kKeyY * camera.viewUpY + kKeyZ * camera.viewUpZ;
 rig.sunDirView[2] = -(kKeyX * camera.viewForwardX + kKeyY * camera.viewForwardY + kKeyZ * camera.viewForwardZ);
 rig.fillDirView[0] = -kKeyX * camera.viewRightX + kKeyY * camera.viewRightY - kKeyZ * camera.viewRightZ;
 rig.fillDirView[1] = -kKeyX * camera.viewUpX + kKeyY * camera.viewUpY - kKeyZ * camera.viewUpZ;
 rig.fillDirView[2] = -(-kKeyX * camera.viewForwardX + kKeyY * camera.viewForwardY - kKeyZ * camera.viewForwardZ);
 rig.sunColor[0] = rig.sunColor[1] = rig.sunColor[2] = 1.0f;
 rig.sunIntensity = 0.6f;
 rig.fillIntensity = 0.6f;
 rig.ambient[0] = rig.ambient[1] = rig.ambient[2] = 0.4f;
 gl::engine_pipeline::setWorldLight(rig);
}
void bindTerrainTexture(int terrainTextureId) {
 if(terrainTextureId < 0) {
  return;
 }
 RenderSystem::activeTexture(gl::tex::Texture0);
 RenderSystem::enableTexture();
 RenderSystem::bindTexture(static_cast<unsigned int>(terrainTextureId));
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
void drawSolidTerrain(
    WorldRenderer& worldRenderer, LivingEntity& camera, float tickDelta, int terrainTextureId, bool ambientOcclusion, unsigned int lightmapTexture) {
 bindTerrainTexture(terrainTextureId);
 if(lightmapTexture != 0) {
  RenderSystem::activeTexture(0x84C1); // GL_TEXTURE1
  RenderSystem::enableTexture();
  RenderSystem::bindTexture(lightmapTexture);
  RenderSystem::activeTexture(0x84C0); // GL_TEXTURE0
 }
 {
  const RenderPassScope solidScope(RenderType::solid());
  RenderSystem::color4f(1.0f, 1.0f, 1.0f, 1.0f);
  worldRenderer.render(camera, 0, static_cast<double>(tickDelta));
 }
 if(lightmapTexture != 0) {
  RenderSystem::activeTexture(0x84C1);
  RenderSystem::disableTexture();
  RenderSystem::activeTexture(0x84C0);
 }
}
void drawTranslucentTerrain(WorldRenderer& worldRenderer,
                            LivingEntity& camera,
                            float tickDelta,
                            int terrainTextureId,
                            bool fancyGraphics,
                            bool ambientOcclusion,
                            unsigned int lightmapTexture) {
 bindTerrainTexture(terrainTextureId);
 if(lightmapTexture != 0) {
  RenderSystem::activeTexture(0x84C1); // GL_TEXTURE1
  RenderSystem::enableTexture();
  RenderSystem::bindTexture(lightmapTexture);
  RenderSystem::activeTexture(0x84C0); // GL_TEXTURE0
 }
 {
  const RenderPassScope translucentScope(RenderType::translucent());
  if(fancyGraphics) {
   {
    const RenderSystem::ColorMaskScope colorMaskPass(false, false, false, false);
    worldRenderer.render(camera, 1, static_cast<double>(tickDelta), false);
   }
   worldRenderer.renderLastChunks(1, static_cast<double>(tickDelta));
  } else {
   worldRenderer.render(camera, 1, static_cast<double>(tickDelta));
  }
 }
 if(lightmapTexture != 0) {
  RenderSystem::activeTexture(0x84C1);
  RenderSystem::disableTexture();
  RenderSystem::activeTexture(0x84C0);
 }
 RenderSystem::depthMask(true);
 RenderSystem::cullBackFaces();
 RenderSystem::disableBlend();
 RenderSystem::alphaTest(0.5f);
}
void renderBlockOverlay(WorldRenderer& worldRenderer,
                        net::minecraft::client::Minecraft* client,
                        PlayerEntity& player,
                        float tickDelta) {
 RenderSystem::alphaTest(0.0f);
 const ItemStack hand = selectedItemOrEmpty(&player);
 worldRenderer.renderMiningProgress(&player, *client->crosshairTarget, 0, hand, tickDelta);
 worldRenderer.renderBlockOutline(&player, *client->crosshairTarget, 0, hand, tickDelta);
 RenderSystem::alphaTest(0.5f);
}
} // namespace
void GameRenderer::renderFrame(float tickDelta, std::int64_t /*timeNs*/) {
 if(client == nullptr) {
  return;
 }
 const int width = std::max(1, client->displayWidth);
 const int height = std::max(1, client->displayHeight);
 renderToCurrentTarget(tickDelta, FrameRenderCamera{}, getFov(tickDelta), width, height, false);
 if(zoom == 1.0) {
  RenderSystem::clear(gl::attrib::DepthBufferBit);
  renderFirstPersonHand(tickDelta);
 }
}
void GameRenderer::renderToCurrentTarget(float tickDelta,
                                         const FrameRenderCamera& cameraFrame,
                                         float fov,
                                         int viewportWidth,
                                         int viewportHeight,
                                         bool renderCameraEntity,
                                         bool captureWorldDepth) {
 if(client == nullptr) {
  return;
 }
 updateLightmap(tickDelta);
 RenderSystem::viewport(0, 0, viewportWidth, viewportHeight);
 RenderSystem::cullBackFaces();
 RenderSystem::depthTest();
 RenderSystem::depthMask(true);
 RenderSystem::alphaTest(0.5f);
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
 frameCamera_ = cameraFrame;
 worldRenderer->setCamera(camera);
 worldRenderer->setRenderCameraEntity(renderCameraEntity);
 if(!frameCamera_.customView) {
   const float rollAmount = prevCameraRollAmount + (cameraRollAmount - prevCameraRollAmount) * tickDelta;
   
   const double eyeOffset = static_cast<double>(camera->standingEyeHeight - 1.62f);
   frameCamera_.x = camera->lastTickX + (camera->x - camera->lastTickX) * static_cast<double>(tickDelta);
   frameCamera_.y = camera->lastTickY + (camera->y - camera->lastTickY) * static_cast<double>(tickDelta) - eyeOffset;
   frameCamera_.z = camera->lastTickZ + (camera->z - camera->lastTickZ) * static_cast<double>(tickDelta);
   frameCamera_.yaw = camera->prevYaw + (camera->yaw - camera->prevYaw) * tickDelta;
   frameCamera_.pitch = camera->prevPitch + (camera->pitch - camera->prevPitch) * tickDelta;
   frameCamera_.roll = rollAmount;
   frameCamera_.customView = false;
   frameCamera_.hideFirstPersonHand = false;

   mod::CameraSetupEvent cameraEvent{};
   cameraEvent.camera = camera;
   cameraEvent.tickDelta = tickDelta;
    cameraEvent.frame = &frameCamera_;
    net::minecraft::mod::runtime::luaHookCameraSetup(cameraEvent);
  }
  RenderCameraState::instance().setFrame(frameCamera_);
 worldRenderer->setFrameRenderCamera(frameCamera_.x, frameCamera_.y, frameCamera_.z);
 if(!renderCameraEntity) {
  client->world->setChunkCacheCenterFromBlockPos(MathHelper::floor(frameCamera_.x),
                                                 MathHelper::floor(frameCamera_.z));
 }
 const option::ResolvedRenderOptions resolvedOptions = option::resolve(client->options);
 const bool ambientOcclusion = client->options.ao;
 const bool fancyGraphics = client->options.fancyGraphics;
 const int terrainTextureId = client->textureManager.getTextureId("/terrain.png");
 const AtmosphereContext atmosphereCtx = makeAtmosphereContext(client, camera, ticks);
 updateSkyAndFogColors(tickDelta);
 debug::RenderProfiler::instance().setEnabled(client->options.debugHud);
 const ProfilerFrame profilerFrame(client->options.debugHud && !frameCamera_.shadowPass && !renderCameraEntity);
 RenderSystem::clear(gl::attrib::ColorBufferBit | gl::attrib::DepthBufferBit);
 renderWorld(tickDelta, fov);
 {
  // The view matrix is rigid (rotations + translations), so the actual GL
  // eye in camera-relative space is -R^T * t. The parametric camera x/y/z
  // differs from this by the first-person view offset and view bobbing.
  const float* v = net::minecraft::util::math::g_modelView.top().data();
  const float* p = net::minecraft::util::math::g_projection.top().data();
  for(int i = 0; i < 3; ++i) {
   const double e = -(static_cast<double>(v[i * 4 + 0]) * v[12] + static_cast<double>(v[i * 4 + 1]) * v[13] +
                      static_cast<double>(v[i * 4 + 2]) * v[14]);
   (i == 0 ? frameCamera_.eyeX : i == 1 ? frameCamera_.eyeY
                                        : frameCamera_.eyeZ) =
       (i == 0 ? frameCamera_.x : i == 1 ? frameCamera_.y
                                         : frameCamera_.z) +
       e;
  }
  frameCamera_.viewRightX = v[0];
  frameCamera_.viewRightY = v[4];
  frameCamera_.viewRightZ = v[8];
  frameCamera_.viewUpX = v[1];
  frameCamera_.viewUpY = v[5];
  frameCamera_.viewUpZ = v[9];
  frameCamera_.viewForwardX = -v[2];
  frameCamera_.viewForwardY = -v[6];
  frameCamera_.viewForwardZ = -v[10];
  frameCamera_.projectionX = p[0];
  frameCamera_.projectionY = p[5];
  RenderCameraState::instance().setFrame(frameCamera_);
 }
 if(client->options.frustumCulling) {
  Frustum::getInstance().compute();
 }
 // GL_FOG must be enabled before the sky/fog setup below, not after: fog
 // params (color/mode/density) are inert while the capability is off, so
 // enabling it late here meant the sky dome always rendered unfogged and
 // painted over the murky underwater clear color, making underwater fog
 // invisible outside of solid, already-fogged terrain.
 setFogEnabled(!frameCamera_.shadowPass);
 if(!frameCamera_.shadowPass && resolvedOptions.viewDistanceSetting < 2 && resolvedOptions.renderSky) {
  const debug::RenderProfiler::Scope skyScope(debug::RenderStage::Sky);
  applyFog(-1);
  if(client->world->dimension != nullptr && !client->world->dimension->isNether) {
   atmosphere::renderSkyDome(atmosphereCtx, tickDelta);
  }
  RenderSystem::alphaTest(0.1f);
 }
 if(!frameCamera_.shadowPass) {
  applyFog(1);
 }
 if(ambientOcclusion) {
 }
 FrustumCuller frustumCuller;
 FrustumCuller* activeCuller = nullptr;
 if(client->options.frustumCulling) {
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
 // Single per-frame lighting block every world program reads: the world sun transformed
 // into the camera's view basis, plus a flat ambient. Replaces the old two-directional rig.
 gl::engine_pipeline::WorldLightUniforms worldLight;
 if(atmosphereCtx.world != nullptr) {
  const auto& sun = atmosphereCtx.world->lightRegistry().sun();
  worldLight.sunDirView[0] = sun.directionX * frameCamera_.viewRightX +
                             sun.directionY * frameCamera_.viewRightY +
                             sun.directionZ * frameCamera_.viewRightZ;
  worldLight.sunDirView[1] = sun.directionX * frameCamera_.viewUpX +
                             sun.directionY * frameCamera_.viewUpY +
                             sun.directionZ * frameCamera_.viewUpZ;
  worldLight.sunDirView[2] = -(sun.directionX * frameCamera_.viewForwardX +
                               sun.directionY * frameCamera_.viewForwardY +
                               sun.directionZ * frameCamera_.viewForwardZ);
  worldLight.sunColor[0] = sun.red;
  worldLight.sunColor[1] = sun.green;
  worldLight.sunColor[2] = sun.blue;
  worldLight.sunIntensity = sun.intensity;
  float skyIntensity = client->world->calculateSkyLightIntensity(tickDelta);
  worldLight.ambient[0] = worldLight.ambient[1] = worldLight.ambient[2] =
      0.4f * (1.0f - skyIntensity * 1.5f);
  worldLight.worldTime = static_cast<float>(ticks) + tickDelta;
  worldLight.brightness = client != nullptr ? client->options.brightness : 0.0f;
  gl::engine_pipeline::setWorldLight(worldLight);
 }
 if(frameCamera_.shadowPass) {
  setFogEnabled(false);
  drawSolidTerrain(*worldRenderer, *camera, tickDelta, terrainTextureId, ambientOcclusion, lightmapTexture_);
  if(frameCamera_.shadowEntities) {
   render::RenderSystem::enableLighting();
   const Vec3d shadowCameraPos{frameCamera_.x, frameCamera_.y, frameCamera_.z};
   worldRenderer->renderEntities(shadowCameraPos, activeCuller, tickDelta);
   render::RenderSystem::disableLighting();
   // Mod entities draw their visuals from Lua world_render hooks rather
   // than native renderers, so the depth map needs the Entities-stage
   // events too or Lua-drawn models never cast shader shadows.
   mod::WorldRenderEvent shadowEntitiesEvent{
       atmosphereCtx.world,
       atmosphereCtx.camera,
       tickDelta,
       mod::WorldRenderStage::Entities,
       mod::RenderHookMoment::Before,
   };
   shadowEntitiesEvent.vanillaStageRan = true;
   shadowEntitiesEvent.shadowPass = true;
    net::minecraft::mod::runtime::luaHookWorldRender(shadowEntitiesEvent);
    shadowEntitiesEvent.moment = mod::RenderHookMoment::After;
    net::minecraft::mod::runtime::luaHookWorldRender(shadowEntitiesEvent);
   }
   RenderSystem::disableBlend();
  RenderSystem::alphaTest(0.1f);
  RenderSystem::color4f(1.0f, 1.0f, 1.0f, 1.0f);
  return;
 }
 applyFog(0);
 mod::WorldRenderEvent terrainEvent{
     atmosphereCtx.world,
     atmosphereCtx.camera,
     tickDelta,
     mod::WorldRenderStage::OpaqueTerrain,
     mod::RenderHookMoment::Before,
 };
  net::minecraft::mod::runtime::luaHookWorldRender(terrainEvent);
  if(!terrainEvent.cancelVanilla) {
  {
   const debug::RenderProfiler::Scope solidScope(debug::RenderStage::SolidTerrain);
   drawSolidTerrain(*worldRenderer, *camera, tickDelta, terrainTextureId, ambientOcclusion, lightmapTexture_);
  }
  terrainEvent.vanillaStageRan = true;
 }
  terrainEvent.moment = mod::RenderHookMoment::After;
  net::minecraft::mod::runtime::luaHookWorldRender(terrainEvent);
  render::RenderSystem::enableLighting();
  applyEntityLightingRig(frameCamera_, worldLight);
  const Vec3d frameCameraPos{frameCamera_.x, frameCamera_.y, frameCamera_.z};
 mod::WorldRenderEvent entitiesEvent{
     atmosphereCtx.world,
     atmosphereCtx.camera,
     tickDelta,
     mod::WorldRenderStage::Entities,
     mod::RenderHookMoment::Before,
 };
  net::minecraft::mod::runtime::luaHookWorldRender(entitiesEvent);
  if(!entitiesEvent.cancelVanilla) {
  const debug::RenderProfiler::Scope entityScope(debug::RenderStage::Entities);
  worldRenderer->renderEntities(frameCameraPos, activeCuller, tickDelta);
  entitiesEvent.vanillaStageRan = true;
 }
  entitiesEvent.moment = mod::RenderHookMoment::After;
  net::minecraft::mod::runtime::luaHookWorldRender(entitiesEvent);
  RenderSystem::alphaTest(0.1f);
 {
  const debug::RenderProfiler::Scope particleScope(debug::RenderStage::Particles);
  RenderSystem::activeTexture(gl::tex::Texture0);
  RenderSystem::enableTexture();
  client->particleManager.renderLit(camera, tickDelta);
  render::RenderSystem::disableLighting();
  gl::engine_pipeline::setWorldLight(worldLight);
  applyFog(0);
  RenderSystem::enableBlend();
  RenderSystem::blendAlpha();
  client->particleManager.render(camera, tickDelta);
 }
 if(client->crosshairTarget.has_value()) {
  if(auto* player = dynamic_cast<PlayerEntity*>(camera)) {
   if(camera->isInFluid(::net::minecraft::block::material::Material::WATER)) {
    renderBlockOverlay(*worldRenderer, client, *player, tickDelta);
   }
  }
 }
 applyFog(0);
 mod::WorldRenderEvent translucentEvent{
     atmosphereCtx.world,
     atmosphereCtx.camera,
     tickDelta,
     mod::WorldRenderStage::TranslucentTerrain,
     mod::RenderHookMoment::Before,
 };
  net::minecraft::mod::runtime::luaHookWorldRender(translucentEvent);
  if(!translucentEvent.cancelVanilla) {
  const debug::RenderProfiler::Scope translucentScope(debug::RenderStage::TranslucentTerrain);
  drawTranslucentTerrain(*worldRenderer, *camera, tickDelta, terrainTextureId, fancyGraphics, ambientOcclusion, lightmapTexture_);
  translucentEvent.vanillaStageRan = true;
 }
  translucentEvent.moment = mod::RenderHookMoment::After;
  net::minecraft::mod::runtime::luaHookWorldRender(translucentEvent);
  if(client->crosshairTarget.has_value() && zoom == 1.0 &&
    !camera->isInFluid(::net::minecraft::block::material::Material::WATER)) {
  if(auto* player = dynamic_cast<PlayerEntity*>(camera)) {
   renderBlockOverlay(*worldRenderer, client, *player, tickDelta);
  }
 }
 precipitationRenderer.renderPrecipitation(atmosphereCtx, tickDelta);
 {
  setFogEnabled(false);
  applyFog(0);
  setFogEnabled(true);
 }
 mod::WorldRenderEvent cloudEvent{
     atmosphereCtx.world,
     atmosphereCtx.camera,
     tickDelta,
     mod::WorldRenderStage::Clouds,
     mod::RenderHookMoment::Before,
 };
  net::minecraft::mod::runtime::luaHookWorldRender(cloudEvent);
  if(!cloudEvent.cancelVanilla && resolvedOptions.renderClouds) {
  const debug::RenderProfiler::Scope cloudScope(debug::RenderStage::Clouds);
  cloudRenderer.renderClouds(atmosphereCtx, tickDelta);
  cloudEvent.vanillaStageRan = true;
 }
  cloudEvent.moment = mod::RenderHookMoment::After;
  net::minecraft::mod::runtime::luaHookWorldRender(cloudEvent);
  setFogEnabled(false);
 applyFog(1);
 RenderSystem::cullBackFaces();
 RenderSystem::disableBlend();
 RenderSystem::alphaTest(0.1f);
 RenderSystem::color4f(1.0f, 1.0f, 1.0f, 1.0f);
 if(zoom == 1.0 && !renderCameraEntity && !captureWorldDepth) {
  const debug::RenderProfiler::Scope handScope(debug::RenderStage::Hand);
  RenderSystem::clear(gl::attrib::DepthBufferBit);
  renderFirstPersonHand(tickDelta);
 }
 if(!renderCameraEntity) {
  mod::WorldRenderEvent framebufferEvent{
      atmosphereCtx.world,
      atmosphereCtx.camera,
      tickDelta,
      mod::WorldRenderStage::Framebuffer,
      mod::RenderHookMoment::Before,
  };
  net::minecraft::mod::runtime::luaHookWorldRender(framebufferEvent);
  framebufferEvent.vanillaStageRan = !framebufferEvent.cancelVanilla;
  framebufferEvent.moment = mod::RenderHookMoment::After;
  net::minecraft::mod::runtime::luaHookWorldRender(framebufferEvent);
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
 const int width = std::max(1, client->displayWidth);
 const int height = std::max(1, client->displayHeight);
 beginHud(util::uiScale(client->options, width, height), width, height);
}
} // namespace net::minecraft::client::render
