#include "net/minecraft/client/render/entity/LivingEntityRenderer.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include <cmath>
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/mob/CreeperEntity.hpp"
#include "net/minecraft/entity/mob/GhastEntity.hpp"
#include "net/minecraft/entity/mob/GiantEntity.hpp"
#include "net/minecraft/entity/mob/PigZombieEntity.hpp"
#include "net/minecraft/entity/mob/SkeletonEntity.hpp"
#include "net/minecraft/entity/mob/SlimeEntity.hpp"
#include "net/minecraft/entity/mob/SpiderEntity.hpp"
#include "net/minecraft/entity/mob/ZombieEntity.hpp"
#include "net/minecraft/entity/passive/ChickenEntity.hpp"
#include "net/minecraft/entity/passive/CowEntity.hpp"
#include "net/minecraft/entity/passive/PigEntity.hpp"
#include "net/minecraft/entity/passive/SheepEntity.hpp"
#include "net/minecraft/entity/passive/SquidEntity.hpp"
#include "net/minecraft/entity/passive/WolfEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/mod/runtime/LuaEntityBindings.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
namespace net::minecraft::client::render::entity {
namespace {
std::string_view entityTypeName(const net::minecraft::entity::LivingEntity& entity) {
 using namespace net::minecraft::entity;
 if(dynamic_cast<const player::PlayerEntity*>(&entity)) {
  return "player";
 }
 if(dynamic_cast<const mob::ZombieEntity*>(&entity)) {
  return "zombie";
 }
 if(dynamic_cast<const mob::SkeletonEntity*>(&entity)) {
  return "skeleton";
 }
 if(dynamic_cast<const mob::CreeperEntity*>(&entity)) {
  return "creeper";
 }
 if(dynamic_cast<const mob::PigZombieEntity*>(&entity)) {
  return "pigzombie";
 }
 if(dynamic_cast<const mob::SpiderEntity*>(&entity)) {
  return "spider";
 }
 if(dynamic_cast<const mob::SlimeEntity*>(&entity)) {
  return "slime";
 }
 if(dynamic_cast<const mob::GhastEntity*>(&entity)) {
  return "ghast";
 }
 if(dynamic_cast<const mob::GiantEntity*>(&entity)) {
  return "giant";
 }
 if(dynamic_cast<const passive::PigEntity*>(&entity)) {
  return "pig";
 }
 if(dynamic_cast<const passive::CowEntity*>(&entity)) {
  return "cow";
 }
 if(dynamic_cast<const passive::ChickenEntity*>(&entity)) {
  return "chicken";
 }
 if(dynamic_cast<const passive::SheepEntity*>(&entity)) {
  return "sheep";
 }
 if(dynamic_cast<const passive::WolfEntity*>(&entity)) {
  return "wolf";
 }
 if(dynamic_cast<const passive::SquidEntity*>(&entity)) {
  return "squid";
 }
 return "living";
}
} // namespace
LivingEntityRenderer::LivingEntityRenderer(model::EntityModel* modelIn, float shadowRadiusIn) : model(modelIn) {
 shadowRadius = shadowRadiusIn;
}
void LivingEntityRenderer::setDecorationModel(model::EntityModel* modelIn) {
 decorationModel = modelIn;
}
void LivingEntityRenderer::render(
    const net::minecraft::Entity& entity, double x, double y, double z, float /*yaw*/, float tickDelta) {
 const auto* living = dynamic_cast<const net::minecraft::LivingEntity*>(&entity);
 if(living == nullptr || model == nullptr) {
  return;
 }
 render::RenderPassScope passScope(render::RenderType::entityCutout());
 { // entityMatrix must be popped before renderNameTag, which pushes its own unrelated matrix.
  RenderSystem::pushMatrix();
  RenderSystem::disableCull();
  model->handSwingProgress = getHandSwingProgress(*living, tickDelta);
  if(decorationModel != nullptr) {
   decorationModel->handSwingProgress = model->handSwingProgress;
  }
  model->riding = living->hasVehicle();
  if(decorationModel != nullptr) {
   decorationModel->riding = model->riding;
  }
  const float baseBodyYaw = living->lastBodyYaw + (living->bodyYaw - living->lastBodyYaw) * tickDelta;
  const float baseHeadYaw = living->prevYaw + (living->yaw - living->prevYaw) * tickDelta;
  const float baseHeadPitch = living->prevPitch + (living->pitch - living->prevPitch) * tickDelta;
  float limbAngle =
      living->lastWalkAnimationSpeed + (living->walkAnimationSpeed - living->lastWalkAnimationSpeed) * tickDelta;
  float limbDistance = living->walkAnimationProgress - living->walkAnimationSpeed * (1.0f - tickDelta);
  if(limbAngle > 1.0f) {
   limbAngle = 1.0f;
  }
  float bodyYaw = baseBodyYaw;
  float headYawRel = baseHeadYaw - baseBodyYaw;
  float headPitch = baseHeadPitch;
  float poseScale = 1.0f;
  float offsetX = 0.0f;
  float offsetY = 0.0f;
  float offsetZ = 0.0f;
  model->poseActive = false;
  model->partOverrides.clear();
  if(decorationModel != nullptr) {
   decorationModel->poseActive = false;
   decorationModel->partOverrides.clear();
  }
  net::minecraft::mod::EntityRenderPose pose{};
  pose.bodyYaw = bodyYaw;
  pose.headYaw = headYawRel;
  pose.headPitch = headPitch;
  pose.limbSwing = limbAngle;
  pose.limbDistance = limbDistance;
  net::minecraft::mod::runtime::applyRegisteredPoseHooks(*living, tickDelta, pose);
  if(net::minecraft::mod::runtime::hasLuaHook(net::minecraft::mod::runtime::LuaEventId::EntityRender)) {
    net::minecraft::mod::EntityRenderEvent event;
    event.entity = living;
    event.entityId = living->id;
    event.entityType = std::string(entityTypeName(*living));
    event.isPlayer = dynamic_cast<const net::minecraft::entity::player::PlayerEntity*>(living) != nullptr;
    event.tickDelta = tickDelta;
    event.pose = pose;
    net::minecraft::mod::runtime::luaHookEntityRender(event);
   pose = event.pose;
  }
  bodyYaw = pose.bodyYaw;
  headYawRel = pose.headYaw;
  headPitch = pose.headPitch;
  limbAngle = pose.limbSwing;
  limbDistance = pose.limbDistance;
  poseScale = pose.scale;
  offsetX = pose.offsetX;
  offsetY = pose.offsetY;
  offsetZ = pose.offsetZ;
  if(!pose.parts.empty()) {
   model->poseActive = true;
   constexpr float kDegToRad = 0.01745329251994329547f;
   for(const auto& [name, part] : pose.parts) {
    auto& out = model->partOverrides[name];
    out.yaw = std::isnan(part.yaw) ? part.yaw : part.yaw * kDegToRad;
    out.pitch = std::isnan(part.pitch) ? part.pitch : part.pitch * kDegToRad;
    out.roll = std::isnan(part.roll) ? part.roll : part.roll * kDegToRad;
   }
  }
  applyTranslation(*living, x, y, z);
  if(offsetX != 0.0f || offsetY != 0.0f || offsetZ != 0.0f) {
   RenderSystem::translate(offsetX, offsetY, offsetZ);
  }
  const float headBob = getHeadBob(*living, tickDelta);
  applyHandSwingRotation(*living, headBob, bodyYaw, tickDelta);
  constexpr float scaleUnit = 0.0625f;
  RenderSystem::scale(-1.0f, -1.0f, 1.0f);
  applyScale(*living, tickDelta);
  if(poseScale != 1.0f) {
   RenderSystem::scale(poseScale, poseScale, poseScale);
  }
  RenderSystem::translate(0.0f, -24.0f * scaleUnit - 0.0078125f, 0.0f);
  (void)bindDownloadedTexture(living->skinUrl, living->getTexture());
  model->animateModel(const_cast<net::minecraft::LivingEntity&>(*living), limbDistance, limbAngle, tickDelta);
  model->render(limbDistance, limbAngle, headBob, headYawRel, headPitch, scaleUnit);
  auto syncDecorationPose = [&]() {
   if(decorationModel != nullptr) {
    decorationModel->poseActive = model->poseActive;
    decorationModel->partOverrides = model->partOverrides;
   }
  };
  for(int layer = 0; layer < 4; ++layer) {
   if(!bindTexture(*living, layer, tickDelta)) {
    continue;
   }
   if(decorationModel != nullptr) {
    syncDecorationPose();
    decorationModel->render(limbDistance, limbAngle, headBob, headYawRel, headPitch, scaleUnit);
   }
   RenderSystem::disableBlend();
  }
  renderMore(*living, tickDelta);
  const float brightness = living->getBrightnessAtEyes(tickDelta);
  const int overlay = getOverlayColor(*living, brightness, tickDelta);
  if(((overlay >> 24) & 0xFF) > 0 || living->hurtTime > 0 || living->deathTime > 0) {
   render::RenderPassScope overlayScope(render::RenderType::entityCutout());
   RenderSystem::disableCull();
   RenderSystem::disableTexture();
   RenderSystem::enableBlend();
   RenderSystem::blendFunc(0x0302, 0x0303); // GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
   RenderSystem::depthFunc(0x0202); // GL_EQUAL
   if(living->hurtTime > 0 || living->deathTime > 0) {
    RenderSystem::color4f(brightness, 0.0f, 0.0f, 0.4f);
    model->render(limbDistance, limbAngle, headBob, headYawRel, headPitch, scaleUnit);
    for(int layer = 0; layer < 4; ++layer) {
     if(!bindDecorationTexture(*living, layer, tickDelta)) {
      continue;
     }
     RenderSystem::color4f(brightness, 0.0f, 0.0f, 0.4f);
     if(decorationModel != nullptr) {
      syncDecorationPose();
      decorationModel->render(limbDistance, limbAngle, headBob, headYawRel, headPitch, scaleUnit);
     }
    }
   }
   if(((overlay >> 24) & 0xFF) > 0) {
    const float cr = static_cast<float>((overlay >> 16) & 0xFF) / 255.0f;
    const float cg = static_cast<float>((overlay >> 8) & 0xFF) / 255.0f;
    const float cb = static_cast<float>(overlay & 0xFF) / 255.0f;
    const float ca = static_cast<float>((overlay >> 24) & 0xFF) / 255.0f;
    RenderSystem::color4f(cr, cg, cb, ca);
    model->render(limbDistance, limbAngle, headBob, headYawRel, headPitch, scaleUnit);
    for(int layer = 0; layer < 4; ++layer) {
     if(!bindDecorationTexture(*living, layer, tickDelta)) {
      continue;
     }
     RenderSystem::color4f(cr, cg, cb, ca);
     if(decorationModel != nullptr) {
      syncDecorationPose();
      decorationModel->render(limbDistance, limbAngle, headBob, headYawRel, headPitch, scaleUnit);
     }
    }
   }
   RenderSystem::depthTest();
   RenderSystem::disableBlend();
   RenderSystem::enableTexture();
  }
  RenderSystem::cullBackFaces();
  RenderSystem::popMatrix();
 } // pop entityMatrix here, before renderNameTag pushes its own matrix
 renderNameTag(*living, x, y, z);
}
void LivingEntityRenderer::applyTranslation(const net::minecraft::LivingEntity& entity, double x, double y, double z) {
 (void)entity;
 RenderSystem::translate(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
}
void LivingEntityRenderer::applyHandSwingRotation(const net::minecraft::LivingEntity& entity,
                                                  float /*headBob*/,
                                                  float bodyYaw,
                                                  float tickDelta) {
 RenderSystem::rotate(180.0f - bodyYaw, 0.0f, 1.0f, 0.0f);
 if(entity.deathTime > 0) {
  float deathAnim = (static_cast<float>(entity.deathTime) + tickDelta - 1.0f) / 20.0f * 1.6f;
  deathAnim = MathHelper::sqrt(deathAnim);
  if(deathAnim > 1.0f) {
   deathAnim = 1.0f;
  }
  RenderSystem::rotate(deathAnim * getDeathYaw(entity), 0.0f, 0.0f, 1.0f);
 }
}
float LivingEntityRenderer::getHandSwingProgress(const net::minecraft::LivingEntity& entity, float tickDelta) const {
 return entity.getHandSwingProgress(tickDelta);
}
float LivingEntityRenderer::getHeadBob(const net::minecraft::LivingEntity& entity, float tickDelta) const {
 return static_cast<float>(entity.age) + tickDelta;
}
void LivingEntityRenderer::renderMore(const net::minecraft::LivingEntity& entity, float tickDelta) {
 (void)entity;
 (void)tickDelta;
}
bool LivingEntityRenderer::bindDecorationTexture(const net::minecraft::LivingEntity& entity,
                                                 int layer,
                                                 float tickDelta) {
 return bindTexture(entity, layer, tickDelta);
}
bool LivingEntityRenderer::bindTexture(const net::minecraft::LivingEntity& entity, int layer, float tickDelta) {
 (void)entity;
 (void)layer;
 (void)tickDelta;
 return false;
}
float LivingEntityRenderer::getDeathYaw(const net::minecraft::LivingEntity& entity) const {
 (void)entity;
 return 90.0f;
}
int LivingEntityRenderer::getOverlayColor(const net::minecraft::LivingEntity& entity,
                                          float brightness,
                                          float tickDelta) const {
 (void)entity;
 (void)brightness;
 (void)tickDelta;
 return 0;
}
void LivingEntityRenderer::applyScale(const net::minecraft::LivingEntity& entity, float tickDelta) {
 (void)entity;
 (void)tickDelta;
}
void LivingEntityRenderer::renderNameTag(const net::minecraft::LivingEntity& entity, double x, double y, double z) {
 if(dispatcher != nullptr && dispatcher->options().debugHud) {
  renderNameTag(entity, std::to_string(entity.id), x, y, z, 64);
 }
}
void LivingEntityRenderer::renderNameTag(
    const net::minecraft::LivingEntity& entity, const std::string& name, double x, double y, double z, int range) {
 if(dispatcher == nullptr || dispatcher->cameraEntity() == nullptr) {
  return;
 }
 const float distance = entity.getDistance(*dispatcher->cameraEntity());
 if(distance > static_cast<float>(range)) {
  return;
 }
 font::TextRenderer* textRenderer = getTextRenderer();
 if(textRenderer == nullptr) {
  return;
 }
 constexpr float nameScale = 1.6f;
 const float pixelScale = 0.016666668f * nameScale;
 {
  RenderSystem::pushMatrix();
  RenderSystem::translate(static_cast<float>(x), static_cast<float>(y) + 2.3f, static_cast<float>(z));
  RenderSystem::rotate(-dispatcher->yaw_, 0.0f, 1.0f, 0.0f);
  RenderSystem::rotate(dispatcher->pitch_, 1.0f, 0.0f, 0.0f);
  RenderSystem::scale(-pixelScale, -pixelScale, pixelScale);
  render::RenderPassScope passScope(render::RenderType::guiTextured());
  Tessellator& tessellator = Tessellator::INSTANCE;
  int yOffset = 0;
  if(name == "deadmau5") {
   yOffset = -10;
  }
  RenderSystem::disableTexture();
  tessellator.startQuads();
  const int halfWidth = textRenderer->getWidth(name) / 2;
  tessellator.color(0.0f, 0.0f, 0.0f, 0.25f);
  tessellator.vertex(-halfWidth - 1, -1 + yOffset, 0.0);
  tessellator.vertex(-halfWidth - 1, 8 + yOffset, 0.0);
  tessellator.vertex(halfWidth + 1, 8 + yOffset, 0.0);
  tessellator.vertex(halfWidth + 1, -1 + yOffset, 0.0);
  tessellator.draw();
  {
   RenderSystem::enableTexture();
   textRenderer->draw(name, -textRenderer->getWidth(name) / 2, yOffset, 0x20FFFFFF);
   RenderSystem::depthTest();
   RenderSystem::depthMask(true);
   textRenderer->draw(name, -textRenderer->getWidth(name) / 2, yOffset, -1);
  }
  RenderSystem::popMatrix();
 }
}
} // namespace net::minecraft::client::render::entity
