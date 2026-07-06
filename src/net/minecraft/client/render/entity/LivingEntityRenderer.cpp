#include "net/minecraft/client/render/entity/LivingEntityRenderer.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
namespace net::minecraft::client::render::entity {
LivingEntityRenderer::LivingEntityRenderer(model::EntityModel* modelIn, float shadowRadiusIn) : model(modelIn) {
  shadowRadius = shadowRadiusIn;
}
LivingEntityRenderer::~LivingEntityRenderer() {
  delete model;
  delete decorationModel;
  model = nullptr;
  decorationModel = nullptr;
}
void LivingEntityRenderer::setDecorationModel(model::EntityModel* modelIn) {
  decorationModel = modelIn;
}
void LivingEntityRenderer::render(const net::minecraft::Entity& entity, double x, double y, double z, float /*yaw*/,
                                  float tickDelta) {
  const auto* living = dynamic_cast<const net::minecraft::LivingEntity*>(&entity);
  if(living == nullptr || model == nullptr) {
    return;
  }
  gl::GL11::glPushMatrix();
  gl::GL11::glDisable(gl::GL11::GL_CULL_FACE);
  model->handSwingProgress = getHandSwingProgress(*living, tickDelta);
  if(decorationModel != nullptr) {
    decorationModel->handSwingProgress = model->handSwingProgress;
  }
  model->riding = living->hasVehicle();
  if(decorationModel != nullptr) {
    decorationModel->riding = model->riding;
  }
  const float bodyYaw = living->lastBodyYaw + (living->bodyYaw - living->lastBodyYaw) * tickDelta;
  const float headYaw = living->prevYaw + (living->yaw - living->prevYaw) * tickDelta;
  const float headPitch = living->prevPitch + (living->pitch - living->prevPitch) * tickDelta;
  applyTranslation(*living, x, y, z);
  const float headBob = getHeadBob(*living, tickDelta);
  applyHandSwingRotation(*living, headBob, bodyYaw, tickDelta);
  constexpr float scaleUnit = 0.0625f;
  gl::GL11::glScalef(-1.0f, -1.0f, 1.0f);
  applyScale(*living, tickDelta);
  gl::GL11::glTranslatef(0.0f, -24.0f * scaleUnit - 0.0078125f, 0.0f);
  float limbAngle =
      living->lastWalkAnimationSpeed + (living->walkAnimationSpeed - living->lastWalkAnimationSpeed) * tickDelta;
  float limbDistance = living->walkAnimationProgress - living->walkAnimationSpeed * (1.0f - tickDelta);
  if(limbAngle > 1.0f) {
    limbAngle = 1.0f;
  }
  [[maybe_unused]] const bool skinBound = bindDownloadedTexture(living->skinUrl, living->getTexture());
  model->animateModel(const_cast<net::minecraft::LivingEntity&>(*living), limbDistance, limbAngle, tickDelta);
  model->render(limbDistance, limbAngle, headBob, headYaw - bodyYaw, headPitch, scaleUnit);
  for(int layer = 0; layer < 4; ++layer) {
    if(!bindTexture(*living, layer, tickDelta)) {
      continue;
    }
    if(decorationModel != nullptr) {
      decorationModel->render(limbDistance, limbAngle, headBob, headYaw - bodyYaw, headPitch, scaleUnit);
    }
    gl::GL11::glDisable(gl::GL11::GL_BLEND);
  }
  renderMore(*living, tickDelta);
  const float brightness = living->getBrightnessAtEyes(tickDelta);
  const int overlay = getOverlayColor(*living, brightness, tickDelta);
  if(((overlay >> 24) & 0xFF) > 0 || living->hurtTime > 0 || living->deathTime > 0) {
    gl::GL11::glDisable(gl::GL11::GL_TEXTURE_2D);
    gl::GL11::glEnable(gl::GL11::GL_BLEND);
    gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
    gl::GL11::glDepthFunc(gl::GL11::GL_EQUAL);
    if(living->hurtTime > 0 || living->deathTime > 0) {
      gl::GL11::glColor4f(brightness, 0.0f, 0.0f, 0.4f);
      model->render(limbDistance, limbAngle, headBob, headYaw - bodyYaw, headPitch, scaleUnit);
      for(int layer = 0; layer < 4; ++layer) {
        if(!bindDecorationTexture(*living, layer, tickDelta)) {
          continue;
        }
        gl::GL11::glColor4f(brightness, 0.0f, 0.0f, 0.4f);
        if(decorationModel != nullptr) {
          decorationModel->render(limbDistance, limbAngle, headBob, headYaw - bodyYaw, headPitch, scaleUnit);
        }
      }
    }
    if(((overlay >> 24) & 0xFF) > 0) {
      const float cr = static_cast<float>((overlay >> 16) & 0xFF) / 255.0f;
      const float cg = static_cast<float>((overlay >> 8) & 0xFF) / 255.0f;
      const float cb = static_cast<float>(overlay & 0xFF) / 255.0f;
      const float ca = static_cast<float>((overlay >> 24) & 0xFF) / 255.0f;
      gl::GL11::glColor4f(cr, cg, cb, ca);
      model->render(limbDistance, limbAngle, headBob, headYaw - bodyYaw, headPitch, scaleUnit);
      for(int layer = 0; layer < 4; ++layer) {
        if(!bindDecorationTexture(*living, layer, tickDelta)) {
          continue;
        }
        gl::GL11::glColor4f(cr, cg, cb, ca);
        if(decorationModel != nullptr) {
          decorationModel->render(limbDistance, limbAngle, headBob, headYaw - bodyYaw, headPitch, scaleUnit);
        }
      }
    }
    gl::GL11::glDepthFunc(gl::GL11::GL_LEQUAL);
    gl::GL11::glDisable(gl::GL11::GL_BLEND);
    gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
  }
  gl::GL11::glEnable(gl::GL11::GL_CULL_FACE);
  gl::GL11::glPopMatrix();
  renderNameTag(*living, x, y, z);
}
void LivingEntityRenderer::applyTranslation(const net::minecraft::LivingEntity& entity, double x, double y, double z) {
  (void)entity;
  gl::GL11::glTranslatef(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
}
void LivingEntityRenderer::applyHandSwingRotation(const net::minecraft::LivingEntity& entity, float /*headBob*/,
                                                  float bodyYaw, float tickDelta) {
  gl::GL11::glRotatef(180.0f - bodyYaw, 0.0f, 1.0f, 0.0f);
  if(entity.deathTime > 0) {
    float deathAnim = (static_cast<float>(entity.deathTime) + tickDelta - 1.0f) / 20.0f * 1.6f;
    deathAnim = MathHelper::sqrt(deathAnim);
    if(deathAnim > 1.0f) {
      deathAnim = 1.0f;
    }
    gl::GL11::glRotatef(deathAnim * getDeathYaw(entity), 0.0f, 0.0f, 1.0f);
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
bool LivingEntityRenderer::bindDecorationTexture(const net::minecraft::LivingEntity& entity, int layer, float tickDelta) {
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
int LivingEntityRenderer::getOverlayColor(const net::minecraft::LivingEntity& entity, float brightness,
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
void LivingEntityRenderer::renderNameTag(const net::minecraft::LivingEntity& entity, const std::string& name, double x,
                                         double y, double z, int range) {
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
  gl::GL11::glPushMatrix();
  gl::GL11::glTranslatef(static_cast<float>(x), static_cast<float>(y) + 2.3f, static_cast<float>(z));
  gl::GL11::glNormal3f(0.0f, 1.0f, 0.0f);
  gl::GL11::glRotatef(-dispatcher->yaw_, 0.0f, 1.0f, 0.0f);
  gl::GL11::glRotatef(dispatcher->pitch_, 1.0f, 0.0f, 0.0f);
  gl::GL11::glScalef(-pixelScale, -pixelScale, pixelScale);
  const gl::AttribGuard labelState(gl::GL11::GL_ENABLE_BIT | gl::GL11::GL_CURRENT_BIT | gl::GL11::GL_TEXTURE_BIT |
                                   gl::GL11::GL_DEPTH_BUFFER_BIT);
  gl::GL11::glDepthMask(false);
  gl::GL11::glDisable(gl::GL11::GL_DEPTH_TEST);
  gl::GL11::glEnable(gl::GL11::GL_BLEND);
  gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
  Tessellator& tessellator = Tessellator::INSTANCE;
  int yOffset = 0;
  if(name == "deadmau5") {
    yOffset = -10;
  }
  gl::GL11::glDisable(gl::GL11::GL_TEXTURE_2D);
  tessellator.startQuads();
  const int halfWidth = textRenderer->getWidth(name) / 2;
  tessellator.color(0.0f, 0.0f, 0.0f, 0.25f);
  tessellator.vertex(-halfWidth - 1, -1 + yOffset, 0.0);
  tessellator.vertex(-halfWidth - 1, 8 + yOffset, 0.0);
  tessellator.vertex(halfWidth + 1, 8 + yOffset, 0.0);
  tessellator.vertex(halfWidth + 1, -1 + yOffset, 0.0);
  tessellator.draw();
  gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
  textRenderer->draw(name, -textRenderer->getWidth(name) / 2, yOffset, 0x20FFFFFF);
  textRenderer->draw(name, -textRenderer->getWidth(name) / 2, yOffset, -1);
  gl::GL11::glPopMatrix();
}
} // namespace net::minecraft::client::render::entity
