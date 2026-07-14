#include "net/minecraft/client/render/entity/PlayerEntityRenderer.hpp"
#include <optional>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/item/HeldItemRenderer.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ArmorItem.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
namespace net::minecraft::client::render::entity {
namespace {
constexpr const char* kArmorTextureNames[] = {"cloth", "chain", "iron", "diamond", "gold"};
void configureArmShape(model::BipedEntityModel* body,
                       model::BipedEntityModel* armorOuter,
                       model::BipedEntityModel* armorInner,
                       bool slimArms) {
  body->configureArms(slimArms);
  armorOuter->configureArms(slimArms);
  armorInner->configureArms(slimArms);
}
bool resolveSlimArms(const net::minecraft::PlayerEntity& player, EntityRenderDispatcher* dispatcher) {
  if(dispatcher == nullptr || dispatcher->textureManager() == nullptr || player.skinUrl.empty()) {
    return player.slimArms;
  }
  if(const std::optional<bool> downloadedSlim = dispatcher->textureManager()->skinSlimArms(player.skinUrl)) {
    return *downloadedSlim;
  }
  return player.slimArms;
}
} // namespace
PlayerEntityRenderer::PlayerEntityRenderer()
    : LivingEntityRenderer(new model::BipedEntityModel(0.0f), 0.5f),
      bipedModel(static_cast<model::BipedEntityModel*>(model.get())),
      armor1(new model::BipedEntityModel(1.0f)),
      armor2(new model::BipedEntityModel(0.5f)) {
}
void PlayerEntityRenderer::render(
    const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta) {
  const auto* player = dynamic_cast<const net::minecraft::PlayerEntity*>(&entity);
  if(player == nullptr) {
    LivingEntityRenderer::render(entity, x, y, z, yaw, tickDelta);
    return;
  }
  const ItemStack* handStack = player->inventory.getSelectedItem();
  configureArmShape(bipedModel, armor1.get(), armor2.get(), resolveSlimArms(*player, dispatcher));
  bipedModel->rightArmPose = handStack != nullptr && !handStack->empty();
  armor2->rightArmPose = bipedModel->rightArmPose;
  armor1->rightArmPose = bipedModel->rightArmPose;
  armor2->sneaking = bipedModel->sneaking = player->isSneaking();
  armor1->sneaking = bipedModel->sneaking;
  double renderY = y - static_cast<double>(player->standingEyeHeight);
  if(player->isSneaking() &&
     dynamic_cast<const ::net::minecraft::entity::player::ClientPlayerEntity*>(player) == nullptr) {
    renderY -= 0.125;
  }
  LivingEntityRenderer::render(entity, x, renderY, z, yaw, tickDelta);
  bipedModel->sneaking = false;
  armor2->sneaking = false;
  armor1->sneaking = false;
  bipedModel->rightArmPose = false;
  armor2->rightArmPose = false;
  armor1->rightArmPose = false;
}
void PlayerEntityRenderer::renderHand() {
  if(bipedModel == nullptr) {
    return;
  }
  if(const auto* player = dynamic_cast<const net::minecraft::PlayerEntity*>(
         dispatcher != nullptr ? dispatcher->cameraEntity() : nullptr)) {
    configureArmShape(bipedModel, armor1.get(), armor2.get(), resolveSlimArms(*player, dispatcher));
  }
  bipedModel->handSwingProgress = 0.0f;
  bipedModel->setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0625f);
  bipedModel->rightArm.render(0.0625f);
}
bool PlayerEntityRenderer::bindTexture(const net::minecraft::LivingEntity& entity, int layer, float tickDelta) {
  (void)tickDelta;
  const auto* player = dynamic_cast<const net::minecraft::PlayerEntity*>(&entity);
  if(player == nullptr || layer < 0 || layer >= 4) {
    return false;
  }
  const ItemStack helmetStack = player->inventory.armor[static_cast<std::size_t>(3 - layer)];
  if(helmetStack.empty()) {
    return false;
  }
  Item* item = helmetStack.getItem();
  auto* armorItem = dynamic_cast<::net::minecraft::item::ArmorItem*>(item);
  if(armorItem == nullptr) {
    return false;
  }
  const int suffix = layer == 2 ? 2 : 1;
  EntityRenderer::bindTexture(std::string("/armor/") + kArmorTextureNames[armorItem->getTextureIndex()] + "_" +
                              std::to_string(suffix) + ".png");
  model::BipedEntityModel* armorModel = layer == 2 ? armor2.get() : armor1.get();
  armorModel->head.visible = layer == 0;
  armorModel->hat.visible = layer == 0;
  armorModel->body.visible = layer == 1 || layer == 2;
  armorModel->rightArm.visible = layer == 1;
  armorModel->leftArm.visible = layer == 1;
  armorModel->rightLeg.visible = layer == 2 || layer == 3;
  armorModel->leftLeg.visible = layer == 2 || layer == 3;
  setDecorationModel(armorModel);
  return true;
}
void PlayerEntityRenderer::applyScale(const net::minecraft::LivingEntity& entity, float tickDelta) {
  (void)entity;
  (void)tickDelta;
  gl::scalef(0.9375f, 0.9375f, 0.9375f);
}
void PlayerEntityRenderer::applyTranslation(const net::minecraft::LivingEntity& entity, double x, double y, double z) {
  const auto* player = dynamic_cast<const net::minecraft::PlayerEntity*>(&entity);
  if(player != nullptr && player->isAlive() && player->isSleeping()) {
    LivingEntityRenderer::applyTranslation(entity,
                                           x + static_cast<double>(player->sleepOffsetX),
                                           y + static_cast<double>(player->sleepOffsetY),
                                           z + static_cast<double>(player->sleepOffsetZ));
    return;
  }
  LivingEntityRenderer::applyTranslation(entity, x, y, z);
}
void PlayerEntityRenderer::applyHandSwingRotation(const net::minecraft::LivingEntity& entity,
                                                  float headBob,
                                                  float bodyYaw,
                                                  float tickDelta) {
  const auto* player = dynamic_cast<const net::minecraft::PlayerEntity*>(&entity);
  if(player != nullptr && player->isAlive() && player->isSleeping()) {
    gl::rotatef(player->getSleepingRotation(), 0.0f, 1.0f, 0.0f);
    gl::rotatef(getDeathYaw(entity), 0.0f, 0.0f, 1.0f);
    gl::rotatef(270.0f, 0.0f, 1.0f, 0.0f);
    return;
  }
  LivingEntityRenderer::applyHandSwingRotation(entity, headBob, bodyYaw, tickDelta);
}
void PlayerEntityRenderer::renderNameTag(const net::minecraft::LivingEntity& entity, double x, double y, double z) {
  const auto* player = dynamic_cast<const net::minecraft::PlayerEntity*>(&entity);
  if(player == nullptr || dispatcher == nullptr || dispatcher->cameraEntity() == nullptr) {
    return;
  }
  if(!Minecraft::isDisplayGui() || player == dispatcher->cameraEntity()) {
    return;
  }
  constexpr float nameScale = 1.6f;
  const float pixelScale = 0.016666668f * nameScale;
  const float distance = player->getDistance(*dispatcher->cameraEntity());
  const float maxDistance = player->isSneaking() ? 32.0f : 64.0f;
  if(distance >= maxDistance) {
    return;
  }
  if(!player->isSneaking()) {
    if(player->isSleeping()) {
      LivingEntityRenderer::renderNameTag(entity, player->name, x, y - 1.5, z, 64);
    } else {
      LivingEntityRenderer::renderNameTag(entity, player->name, x, y, z, 64);
    }
    return;
  }
  font::TextRenderer* textRenderer = getTextRenderer();
  if(textRenderer == nullptr) {
    return;
  }
  gl::pushMatrix();
  gl::translatef(static_cast<float>(x), static_cast<float>(y) + 2.3f, static_cast<float>(z));
  gl::normal3f(0.0f, 1.0f, 0.0f);
  gl::rotatef(-dispatcher->yaw_, 0.0f, 1.0f, 0.0f);
  gl::rotatef(dispatcher->pitch_, 1.0f, 0.0f, 0.0f);
  gl::scalef(-pixelScale, -pixelScale, pixelScale);
  const gl::preset::LabelDraw labelCaps;
  Tessellator& tessellator = Tessellator::INSTANCE;
  {
    const gl::preset::LabelTextureOff labelBackgroundCaps;
    tessellator.startQuads();
    const int halfWidth = textRenderer->getWidth(player->name) / 2;
    tessellator.color(0.0f, 0.0f, 0.0f, 0.25f);
    tessellator.vertex(-halfWidth - 1, -1.0, 0.0);
    tessellator.vertex(-halfWidth - 1, 8.0, 0.0);
    tessellator.vertex(halfWidth + 1, 8.0, 0.0);
    tessellator.vertex(halfWidth + 1, -1.0, 0.0);
    tessellator.draw();
  }
  {
    const gl::preset::GuiTextureOn labelTextCaps;
    textRenderer->draw(player->name, -textRenderer->getWidth(player->name) / 2, 0, 0x20FFFFFF);
  }
  gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
  gl::popMatrix();
}
void PlayerEntityRenderer::renderMore(const net::minecraft::LivingEntity& entity, float tickDelta) {
  const auto* player = dynamic_cast<const net::minecraft::PlayerEntity*>(&entity);
  if(player == nullptr || bipedModel == nullptr || dispatcher == nullptr) {
    return;
  }
  item::HeldItemRenderer* heldItemRenderer = dispatcher->heldItemRenderer();
  if(heldItemRenderer == nullptr) {
    return;
  }
  const ItemStack helmetStack = player->inventory.armor[3];
  if(!helmetStack.empty() && helmetStack.itemId < 256) {
    gl::pushMatrix();
    bipedModel->head.transform(0.0625f);
    Block* block = Block::BLOCKS[static_cast<std::size_t>(helmetStack.itemId)];
    if(block != nullptr && block::BlockRenderManager::isSideLit(block->getRenderType())) {
      constexpr float scale = 0.625f;
      gl::translatef(0.0f, -0.25f, 0.0f);
      gl::rotatef(180.0f, 0.0f, 1.0f, 0.0f);
      gl::scalef(scale, -scale, scale);
    }
    heldItemRenderer->renderItem(*player, helmetStack);
    gl::popMatrix();
  }
  if(player->name == "deadmau5" && bindDownloadedTexture(player->skinUrl, "")) {
    for(int ear = 0; ear < 2; ++ear) {
      const float headYaw = player->prevYaw + (player->yaw - player->prevYaw) * tickDelta -
                            (player->lastBodyYaw + (player->bodyYaw - player->lastBodyYaw) * tickDelta);
      const float headPitch = player->prevPitch + (player->pitch - player->prevPitch) * tickDelta;
      gl::pushMatrix();
      gl::rotatef(headYaw, 0.0f, 1.0f, 0.0f);
      gl::rotatef(headPitch, 1.0f, 0.0f, 0.0f);
      gl::translatef(0.375f * static_cast<float>(ear * 2 - 1), 0.0f, 0.0f);
      gl::translatef(0.0f, -0.375f, 0.0f);
      gl::rotatef(-headPitch, 1.0f, 0.0f, 0.0f);
      gl::rotatef(-headYaw, 0.0f, 1.0f, 0.0f);
      constexpr float earScale = 1.3333334f;
      gl::scalef(earScale, earScale, earScale);
      bipedModel->renderEars(0.0625f);
      gl::popMatrix();
    }
  }
  if(bindDownloadedTexture(player->playerCapeUrl, "")) {
    gl::pushMatrix();
    gl::translatef(0.0f, 0.0f, 0.125f);
    const double capeDx = player->prevCapeX + (player->capeX - player->prevCapeX) * static_cast<double>(tickDelta) -
                          (player->prevX + (player->x - player->prevX) * static_cast<double>(tickDelta));
    const double capeDy = player->prevCapeY + (player->capeY - player->prevCapeY) * static_cast<double>(tickDelta) -
                          (player->prevY + (player->y - player->prevY) * static_cast<double>(tickDelta));
    const double capeDz = player->prevCapeZ + (player->capeZ - player->prevCapeZ) * static_cast<double>(tickDelta) -
                          (player->prevZ + (player->z - player->prevZ) * static_cast<double>(tickDelta));
    const float bodyYaw = player->lastBodyYaw + (player->bodyYaw - player->lastBodyYaw) * tickDelta;
    const double sinYaw = MathHelper::sin(bodyYaw * 3.14159265f / 180.0f);
    const double cosYaw = -MathHelper::cos(bodyYaw * 3.14159265f / 180.0f);
    float capeLift = static_cast<float>(capeDy) * 10.0f;
    if(capeLift < -6.0f) {
      capeLift = -6.0f;
    }
    if(capeLift > 32.0f) {
      capeLift = 32.0f;
    }
    float capeAngle = static_cast<float>(capeDx * sinYaw + capeDz * cosYaw) * 100.0f;
    float capeTwist = static_cast<float>(capeDx * cosYaw - capeDz * sinYaw) * 100.0f;
    if(capeAngle < 0.0f) {
      capeAngle = 0.0f;
    }
    const float stepBob =
        player->prevStepBobbingAmount + (player->stepBobbingAmount - player->prevStepBobbingAmount) * tickDelta;
    capeLift += MathHelper::sin((player->prevHorizontalSpeed +
                                 (player->horizontalSpeed - player->prevHorizontalSpeed) * tickDelta) *
                                6.0f) *
                32.0f * stepBob;
    if(player->isSneaking()) {
      capeLift += 25.0f;
    }
    gl::rotatef(6.0f + capeAngle / 2.0f + capeLift, 1.0f, 0.0f, 0.0f);
    gl::rotatef(capeTwist / 2.0f, 0.0f, 0.0f, 1.0f);
    gl::rotatef(-capeTwist / 2.0f, 0.0f, 1.0f, 0.0f);
    gl::rotatef(180.0f, 0.0f, 1.0f, 0.0f);
    bipedModel->renderCape(0.0625f);
    gl::popMatrix();
  }
  ItemStack handStack =
      player->inventory.getSelectedItem() != nullptr ? *player->inventory.getSelectedItem() : ItemStack{};
  if(player->fishHook != nullptr) {
    handStack = ItemStack{280, 1, 0};
  }
  if(!handStack.empty()) {
    gl::pushMatrix();
    bipedModel->rightArm.transform(0.0625f);
    gl::translatef(-0.0625f, 0.4375f, 0.0625f);
    if(handStack.itemId < 256) {
      Block* block = Block::BLOCKS[static_cast<std::size_t>(handStack.itemId)];
      if(block != nullptr && block::BlockRenderManager::isSideLit(block->getRenderType())) {
        float scale = 0.5f;
        gl::translatef(0.0f, 0.1875f, -0.3125f);
        gl::rotatef(20.0f, 1.0f, 0.0f, 0.0f);
        gl::rotatef(45.0f, 0.0f, 1.0f, 0.0f);
        scale *= 0.75f;
        gl::scalef(scale, -scale, scale);
      } else if(Item* item = handStack.getItem(); item != nullptr && item->isHandheld()) {
        float scale = 0.625f;
        if(item->isHandheldRod()) {
          gl::rotatef(180.0f, 0.0f, 0.0f, 1.0f);
          gl::translatef(0.0f, -0.125f, 0.0f);
        }
        gl::translatef(0.0f, 0.1875f, 0.0f);
        gl::scalef(scale, -scale, scale);
        gl::rotatef(-100.0f, 1.0f, 0.0f, 0.0f);
        gl::rotatef(45.0f, 0.0f, 1.0f, 0.0f);
      } else {
        const float scale = 0.375f;
        gl::translatef(0.25f, 0.1875f, -0.1875f);
        gl::scalef(scale, scale, scale);
        gl::rotatef(60.0f, 0.0f, 0.0f, 1.0f);
        gl::rotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gl::rotatef(20.0f, 0.0f, 0.0f, 1.0f);
      }
    } else if(Item* item = handStack.getItem(); item != nullptr && item->isHandheld()) {
      float scale = 0.625f;
      if(item->isHandheldRod()) {
        gl::rotatef(180.0f, 0.0f, 0.0f, 1.0f);
        gl::translatef(0.0f, -0.125f, 0.0f);
      }
      gl::translatef(0.0f, 0.1875f, 0.0f);
      gl::scalef(scale, -scale, scale);
      gl::rotatef(-100.0f, 1.0f, 0.0f, 0.0f);
      gl::rotatef(45.0f, 0.0f, 1.0f, 0.0f);
    } else {
      const float scale = 0.375f;
      gl::translatef(0.25f, 0.1875f, -0.1875f);
      gl::scalef(scale, scale, scale);
      gl::rotatef(60.0f, 0.0f, 0.0f, 1.0f);
      gl::rotatef(-90.0f, 1.0f, 0.0f, 0.0f);
      gl::rotatef(20.0f, 0.0f, 0.0f, 1.0f);
    }
    heldItemRenderer->renderItem(*player, handStack);
    gl::popMatrix();
  }
}
} // namespace net::minecraft::client::render::entity
