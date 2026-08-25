#include "net/minecraft/client/render/item/HeldItemRenderer.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include <algorithm>
#include <cmath>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/pipeline/Pipeline.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/TextureResolve.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/entity/EntityRenderer.hpp"
#include "net/minecraft/client/render/entity/PlayerEntityRenderer.hpp"
#include "net/minecraft/client/render/item/ItemModelRenderer.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/MapItem.hpp"
#include "net/minecraft/item/map/MapState.hpp"
#include "net/minecraft/mod/model/ModModels.hpp"
#include "net/minecraft/registry/TextureRegistry.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
#include "net/minecraft/util/math/MatrixStack.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/light/LightType.hpp"
namespace net::minecraft::client::render::item {
namespace {
constexpr float kPi = 3.14159265358979323846f;
[[nodiscard]] float handSwingProgress(const net::minecraft::entity::LivingEntity& entity, float tickDelta) {
 return entity.getHandSwingProgress(tickDelta);
}
[[nodiscard]] bool usingUserShaderPack(render::GameRenderer* gameRenderer) {
 auto* pipeline = gameRenderer != nullptr ? gameRenderer->shaderPipeline() : nullptr;
 return pipeline != nullptr && pipeline->usingUserPack();
}
void applyVanillaHandLighting() {
 constexpr float kLength = 1.2369317f;
 constexpr float kSunX = 0.2f / kLength;
 constexpr float kSunY = 1.0f / kLength;
 constexpr float kSunZ = -0.7f / kLength;
 render::core::WorldLightUniforms light = render::core::worldLight();
 light.sunDirView[0] = kSunX;
 light.sunDirView[1] = kSunY;
 light.sunDirView[2] = kSunZ;
 light.fillDirView[0] = -kSunX;
 light.fillDirView[1] = kSunY;
 light.fillDirView[2] = -kSunZ;
 light.sunColor[0] = light.sunColor[1] = light.sunColor[2] = 1.0f;
 light.sunIntensity = 0.6f;
 light.fillIntensity = 0.6f;
 light.ambient[0] = light.ambient[1] = light.ambient[2] = 0.4f;
 render::core::setWorldLight(light);
 render::core::setLightingEnabled(true);
}
void renderTexturedOverlay(int textureId) {
 Tessellator& tessellator = Tessellator::INSTANCE;
 constexpr float brightness = 0.1f;
 render::core::setConstColor(brightness, brightness, brightness, 0.5f);
 constexpr float left = -1.0f;
 constexpr float right = 1.0f;
 constexpr float bottom = -1.0f;
 constexpr float top = 1.0f;
 constexpr float depth = -0.5f;
 constexpr float inset = 0.0078125f;
 const float uMax = static_cast<float>(textureId % 16) / 256.0f - inset;
 const float uMin = (static_cast<float>(textureId % 16) + 15.99f) / 256.0f + inset;
 const float vMax = static_cast<float>(textureId / 16) / 256.0f - inset;
 const float vMin = (static_cast<float>(textureId / 16) + 15.99f) / 256.0f + inset;
 tessellator.startQuads();
 tessellator.vertex(left, bottom, depth, uMin, vMin);
 tessellator.vertex(right, bottom, depth, uMax, vMin);
 tessellator.vertex(right, top, depth, uMax, vMax);
 tessellator.vertex(left, top, depth, uMin, vMax);
 tessellator.draw();
 render::core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
}
void renderUnderwaterOverlay(net::minecraft::entity::player::PlayerEntity& player, float tickDelta) {
 Tessellator& tessellator = Tessellator::INSTANCE;
 const float brightness = player.getBrightnessAtEyes(tickDelta);
 render::RenderPassScope scope(render::RenderType::guiTextured());
 render::core::setConstColor(brightness, brightness, brightness, 0.5f);
 constexpr float scroll = 4.0f;
 constexpr float left = -1.0f;
 constexpr float right = 1.0f;
 constexpr float bottom = -1.0f;
 constexpr float top = 1.0f;
 constexpr float depth = -0.5f;
 const float uScroll = -player.yaw / 64.0f;
 const float vScroll = player.pitch / 64.0f;
 tessellator.startQuads();
 tessellator.vertex(right, bottom, depth, scroll + uScroll, scroll + vScroll);
 tessellator.vertex(left, bottom, depth, uScroll, scroll + vScroll);
 tessellator.vertex(left, top, depth, uScroll, vScroll);
 tessellator.vertex(right, top, depth, scroll + uScroll, vScroll);
 tessellator.draw();
 render::core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
}
void renderFireOverlay() {
 Tessellator& tessellator = Tessellator::INSTANCE;
 render::RenderPassScope scope(render::RenderType::guiTextured());
 render::core::setConstColor(1.0f, 1.0f, 1.0f, 0.9f);
 constexpr float size = 1.0f;
 net::minecraft::block::Block* fireBlock = net::minecraft::block::Block::FIRE;
 const float x0 = (0.0f - size) / 2.0f;
 const float x1 = x0 + size;
 const float y0 = 0.0f - size / 2.0f;
 const float y1 = y0 + size;
 constexpr float depth = -0.5f;
 tessellator.startQuads();
 for(int layer = 0; layer < 2; ++layer) {
  const int texture = (fireBlock != nullptr ? fireBlock->textureId : 31) + layer * 16;
  const int uOrigin = (texture & 0xF) << 4;
  const int vOrigin = texture & 0xF0;
  const float uMin = static_cast<float>(uOrigin) / 256.0f;
  const float uMax = (static_cast<float>(uOrigin) + 15.99f) / 256.0f;
  const float vMin = static_cast<float>(vOrigin) / 256.0f;
  const float vMax = (static_cast<float>(vOrigin) + 15.99f) / 256.0f;
  net::minecraft::util::math::Matrix4f model;
  model.set(core::drawPose().data());
  model.translate(static_cast<float>(-(layer * 2 - 1)) * 0.24f, -0.3f, 0.0f);
  model.rotate(static_cast<float>(layer * 2 - 1) * 10.0f, 0.0f, 1.0f, 0.0f);
  struct Corner {
   float x;
   float y;
   float z;
   float u;
   float v;
  };
  const Corner corners[4] = {
      {x0, y0, depth, uMax, vMax},
      {x1, y0, depth, uMin, vMax},
      {x1, y1, depth, uMin, vMin},
      {x0, y1, depth, uMax, vMin},
  };
  for(const Corner& corner : corners) {
   float x = 0.0f;
   float y = 0.0f;
   float z = 0.0f;
   model.transformPoint(corner.x, corner.y, corner.z, x, y, z);
   tessellator.vertex(x, y, z, corner.u, corner.v);
  }
 }
 tessellator.draw();
 render::core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
}
} // namespace
HeldItemRenderer::HeldItemRenderer(net::minecraft::client::Minecraft* minecraftIn)
    : minecraft(minecraftIn),
      mapRenderer(minecraftIn != nullptr ? minecraftIn->textRenderer.get() : nullptr,
                  minecraftIn != nullptr ? &minecraftIn->textureManager : nullptr) {
}
int HeldItemRenderer::skinTexture(const net::minecraft::entity::player::ClientPlayerEntity& player) {
 return minecraft->textureManager.downloadTexture(player.skinUrl, player.getTexture());
}
void HeldItemRenderer::renderItem(const net::minecraft::entity::LivingEntity& entity, const ItemStack& itemStack) {
 if(minecraft == nullptr || itemStack.empty()) {
  return;
 }
 const render::core::RenderedItemScope itemScope(ItemModelRenderer::shaderId(itemStack));
  if(ItemModelRenderer::hasCustomModel(itemStack)) {
   minecraft->textureManager.bindTexture(
       resolveBlockTexture(itemStack.getTextureId(), minecraft->textureManager, ItemModelRenderer::atlasDomain(itemStack))
           .glId);
   const core::ScopedDrawCameraState itemGuard;
   net::minecraft::util::math::MatrixStack pose;
   pose.load(core::drawPose());
   pose.translate(0.0f, -0.3f, 0.0f);
   pose.scale(1.5f, 1.5f, 1.5f);
   pose.rotate(50.0f, 0.0f, 1.0f, 0.0f);
   pose.rotate(335.0f, 0.0f, 0.0f, 1.0f);
   pose.translate(-0.5f, -0.5f, -0.5f);
   core::setDrawPose(pose.top());
   net::minecraft::mod::model::drawLuaItemModel(
       Tessellator::INSTANCE, itemStack, entity.getBrightnessAtEyes(1.0f));
   return;
  }
 if(ItemModelRenderer::rendersAsBlockModel(itemStack)) {
  net::minecraft::block::Block* block = ItemModelRenderer::blockOf(itemStack);
  minecraft->textureManager.bindTexture(minecraft->textureManager.getTextureId("/terrain.png"));
  blockRenderManager.ctx.textureManager = &minecraft->textureManager;
  const bool cullWasEnabled = core::cullEnabled();
  core::disableCull();
  const core::ScopedDrawCameraState itemGuard;
  blockRenderManager.render(*block, itemStack.getDamage(), entity.getBrightnessAtEyes(1.0f));
  if(cullWasEnabled) {
   core::enableCull();
  }
  blockRenderManager.ctx.textureManager = nullptr;
  return;
 }
 minecraft->textureManager.bindTexture(
     resolveBlockTexture(itemStack.getTextureId(), minecraft->textureManager, ItemModelRenderer::atlasDomain(itemStack))
         .glId);
 Tessellator& tessellator = Tessellator::INSTANCE;
 const auto uv = ItemModelRenderer::spriteUv(itemStack);
 const float uMin = static_cast<float>(uv.uMin);
 const float uMax = static_cast<float>(uv.uMax);
 const float vMin = static_cast<float>(uv.vMin);
 const float vMax = static_cast<float>(uv.vMax);
  const core::ScopedDrawCameraState itemGuard;
  net::minecraft::util::math::MatrixStack pose;
  pose.load(core::drawPose());
  pose.translate(0.0f, -0.3f, 0.0f);
  constexpr float itemScale = 1.5f;
  pose.scale(itemScale, itemScale, itemScale);
  pose.rotate(50.0f, 0.0f, 1.0f, 0.0f);
  pose.rotate(335.0f, 0.0f, 0.0f, 1.0f);
  pose.translate(-0.9375f, -0.0625f, 0.0f);
  core::setDrawPose(pose.top());
  net::minecraft::mod::model::drawExtrudedSprite(tessellator, uMin, uMax, vMin, vMax);
}
void HeldItemRenderer::render(float tickDelta) {
 if(minecraft == nullptr || minecraft->player == nullptr || minecraft->world == nullptr) {
  return;
 }
 auto* clientPlayer = dynamic_cast<net::minecraft::entity::player::ClientPlayerEntity*>(minecraft->player);
 if(clientPlayer == nullptr) {
  return;
 }
 const float equipProgress = prevHeight + (height - prevHeight) * tickDelta;
 const float pitch = clientPlayer->prevPitch + (clientPlayer->pitch - clientPlayer->prevPitch) * tickDelta;
 const ItemStack* selectedItem = stack.empty() ? nullptr : &stack;
 const int handX = MathHelper::floor(clientPlayer->x);
 const int handY = MathHelper::floor(clientPlayer->y + static_cast<double>(clientPlayer->getEyeHeight()));
 const int handZ = MathHelper::floor(clientPlayer->z);
 const float rawHandLight = minecraft->world->getLightBrightness(handX, handY, handZ);
 const int handBlockLight = minecraft->world->getBrightness(net::minecraft::LightType::Block, handX, handY, handZ);
 const int handSkyLight = minecraft->world->getBrightness(net::minecraft::LightType::Sky, handX, handY, handZ);
 auto* gameRenderer = minecraft->gameRenderer.get();
 const bool useOldHandLight = gameRenderer == nullptr
                                  ? vanillaPackDefinition().oldHandLight
                                  : gameRenderer->packDefinition().oldHandLight;
  const float colorMult = useOldHandLight ? rawHandLight : 1.0f;
  Tessellator::INSTANCE.light(static_cast<float>(handBlockLight), static_cast<float>(handSkyLight));
  entity::EntityRenderer* entityRenderer = entity::EntityRenderDispatcher::instance().get(*clientPlayer);
  auto* playerRenderer = dynamic_cast<entity::PlayerEntityRenderer*>(entityRenderer);
  render::RenderPassScope firstPersonScope(render::RenderType::hand());
  if(!usingUserShaderPack(gameRenderer)) {
   applyVanillaHandLighting();
  }
  if(selectedItem != nullptr && selectedItem->getItem() != nullptr) {
   const int tint = selectedItem->getItem()->getColorMultiplier(selectedItem->getDamage());
   const float red = static_cast<float>((tint >> 16) & 0xFF) / 255.0f;
   const float green = static_cast<float>((tint >> 8) & 0xFF) / 255.0f;
   const float blue = static_cast<float>(tint & 0xFF) / 255.0f;
   render::core::setConstColor(colorMult * red, colorMult * green, colorMult * blue, 1.0f);
  } else {
   render::core::setConstColor(colorMult, colorMult, colorMult, 1.0f);
  }
 if(selectedItem != nullptr && Item::byRawId(102) != nullptr && selectedItem->itemId == Item::byRawId(102)->id) {
  const core::ScopedDrawCameraState mapGuard;
  net::minecraft::util::math::MatrixStack mapPose;
  mapPose.load(core::drawPose());
  constexpr float scale = 0.8f;
  float swing = handSwingProgress(*clientPlayer, tickDelta);
  float swingSin = MathHelper::sin(swing * static_cast<float>(kPi));
  float swingSqrt = MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi));
  mapPose.translate(-swingSqrt * 0.4f,
                    MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi) * 2.0f) * 0.2f,
                    -swingSin * 0.2f);
  float pitchFactor = 1.0f - pitch / 45.0f + 0.1f;
  pitchFactor = std::clamp(pitchFactor, 0.0f, 1.0f);
  pitchFactor = -MathHelper::cos(pitchFactor * static_cast<float>(kPi)) * 0.5f + 0.5f;
  mapPose.translate(
      0.0f, 0.0f * scale - (1.0f - equipProgress) * 1.2f - pitchFactor * 0.5f + 0.04f, -0.9f * scale);
  mapPose.rotate(90.0f, 0.0f, 1.0f, 0.0f);
  mapPose.rotate(pitchFactor * -85.0f, 0.0f, 0.0f, 1.0f);
  core::setDrawPose(mapPose.top());
  minecraft->textureManager.bindTexture(skinTexture(*clientPlayer));
  if(playerRenderer != nullptr) {
   for(int side = 0; side < 2; ++side) {
    const int mirror = side * 2 - 1;
    const core::ScopedDrawCameraState handGuard;
    net::minecraft::util::math::MatrixStack handPose;
    handPose.load(core::drawPose());
    handPose.translate(0.0f, -0.6f, 1.1f * static_cast<float>(mirror));
    handPose.rotate(static_cast<float>(-45 * mirror), 1.0f, 0.0f, 0.0f);
    handPose.rotate(-90.0f, 0.0f, 0.0f, 1.0f);
    handPose.rotate(59.0f, 0.0f, 0.0f, 1.0f);
    handPose.rotate(static_cast<float>(-65 * mirror), 0.0f, 1.0f, 0.0f);
    core::setDrawPose(handPose.top());
    playerRenderer->renderHand();
   }
  }
  swing = handSwingProgress(*clientPlayer, tickDelta);
  const float swingSin2 = MathHelper::sin(swing * swing * static_cast<float>(kPi));
  swingSqrt = MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi));
  mapPose.rotate(-swingSin2 * 20.0f, 0.0f, 1.0f, 0.0f);
  mapPose.rotate(-swingSqrt * 20.0f, 0.0f, 0.0f, 1.0f);
  mapPose.rotate(-swingSqrt * 80.0f, 1.0f, 0.0f, 0.0f);
  mapPose.scale(0.38f, 0.38f, 0.38f);
  mapPose.rotate(90.0f, 0.0f, 1.0f, 0.0f);
  mapPose.rotate(180.0f, 0.0f, 0.0f, 1.0f);
  mapPose.translate(-1.0f, -1.0f, 0.0f);
  mapPose.scale(0.015625f, 0.015625f, 0.015625f);
  core::setDrawPose(mapPose.top());
  minecraft->textureManager.bindTexture(minecraft->textureManager.getTextureId("/misc/mapbg.png"));
  Tessellator& tessellator = Tessellator::INSTANCE;
  tessellator.startQuads();
  constexpr int border = 7;
  tessellator.vertex(0 - border, 128 + border, 0.0, 0.0, 1.0);
  tessellator.vertex(128 + border, 128 + border, 0.0, 1.0, 1.0);
  tessellator.vertex(128 + border, 0 - border, 0.0, 1.0, 0.0);
  tessellator.vertex(0 - border, 0 - border, 0.0, 0.0, 0.0);
  tessellator.draw();
  if(Item::byRawId(102) != nullptr) {
   ItemStack mapStack = *selectedItem;
   if(auto* mapItem = dynamic_cast<::net::minecraft::item::MapItem*>(Item::byRawId(102))) {
    if(map::MapState* mapState = mapItem->getSavedMapState(mapStack, minecraft->world)) {
     mapRenderer.render(*clientPlayer, minecraft->textureManager, *mapState);
    }
   }
  }
 } else if(selectedItem != nullptr) {
  const core::ScopedDrawCameraState itemGuard;
  net::minecraft::util::math::MatrixStack pose;
  pose.load(core::drawPose());
  constexpr float scale = 0.8f;
  float swing = handSwingProgress(*clientPlayer, tickDelta);
  float swingSin = MathHelper::sin(swing * static_cast<float>(kPi));
  float swingSqrt = MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi));
  pose.translate(-swingSqrt * 0.4f,
                 MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi) * 2.0f) * 0.2f,
                 -swingSin * 0.2f);
  pose.translate(0.7f * scale, -0.65f * scale - (1.0f - equipProgress) * 0.6f, -0.9f * scale);
  pose.rotate(45.0f, 0.0f, 1.0f, 0.0f);
  swing = handSwingProgress(*clientPlayer, tickDelta);
  swingSin = MathHelper::sin(swing * swing * static_cast<float>(kPi));
  swingSqrt = MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi));
  pose.rotate(-swingSin * 20.0f, 0.0f, 1.0f, 0.0f);
  pose.rotate(-swingSqrt * 20.0f, 0.0f, 0.0f, 1.0f);
  pose.rotate(-swingSqrt * 80.0f, 1.0f, 0.0f, 0.0f);
  pose.scale(0.4f, 0.4f, 0.4f);
  if(selectedItem->getItem() != nullptr && selectedItem->getItem()->isHandheldRod()) {
   pose.rotate(180.0f, 0.0f, 1.0f, 0.0f);
  }
  core::setDrawPose(pose.top());
  renderItem(*clientPlayer, *selectedItem);
 } else if(playerRenderer != nullptr) {
  const core::ScopedDrawCameraState handGuard;
  net::minecraft::util::math::MatrixStack pose;
  pose.load(core::drawPose());
  constexpr float scale = 0.8f;
  float swing = handSwingProgress(*clientPlayer, tickDelta);
  float swingSin = MathHelper::sin(swing * static_cast<float>(kPi));
  float swingSqrt = MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi));
  pose.translate(-swingSqrt * 0.3f,
                 MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi) * 2.0f) * 0.4f,
                 -swingSin * 0.4f);
  pose.translate(0.8f * scale, -0.75f * scale - (1.0f - equipProgress) * 0.6f, -0.9f * scale);
  pose.rotate(45.0f, 0.0f, 1.0f, 0.0f);
  swing = handSwingProgress(*clientPlayer, tickDelta);
  swingSin = MathHelper::sin(swing * swing * static_cast<float>(kPi));
  swingSqrt = MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi));
  pose.rotate(swingSqrt * 70.0f, 0.0f, 1.0f, 0.0f);
  pose.rotate(-swingSin * 20.0f, 0.0f, 0.0f, 1.0f);
  minecraft->textureManager.bindTexture(skinTexture(*clientPlayer));
  pose.translate(-1.0f, 3.6f, 3.5f);
  pose.rotate(120.0f, 0.0f, 0.0f, 1.0f);
  pose.rotate(200.0f, 1.0f, 0.0f, 0.0f);
  pose.rotate(-135.0f, 0.0f, 1.0f, 0.0f);
  pose.translate(5.6f, 0.0f, 0.0f);
  core::setDrawPose(pose.top());
  playerRenderer->renderHand();
 }
}
void HeldItemRenderer::renderScreenOverlays(float tickDelta) {
 if(minecraft == nullptr || minecraft->player == nullptr || minecraft->world == nullptr) {
  return;
 }
 render::RenderPassScope overlayScope(render::RenderType::guiTextured());
 if(minecraft->player->isOnFire()) {
  minecraft->textureManager.bindTexture(minecraft->textureManager.getTextureId("/terrain.png"));
  renderFireOverlay();
 }
 if(minecraft->player->isInsideWall()) {
  const int blockX = MathHelper::floor(minecraft->player->x);
  const int blockY = MathHelper::floor(minecraft->player->y);
  const int blockZ = MathHelper::floor(minecraft->player->z);
  minecraft->textureManager.bindTexture(minecraft->textureManager.getTextureId("/terrain.png"));
  int blockId = minecraft->world->getBlockId(blockX, blockY, blockZ);
  if(minecraft->world->shouldSuffocate(blockX, blockY, blockZ)) {
   if(net::minecraft::block::Block* block =
          net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
      block != nullptr) {
    renderTexturedOverlay(block->getTexture(2));
   }
  } else {
   for(int i = 0; i < 8; ++i) {
    const float offsetX = (static_cast<float>((i >> 0) % 2) - 0.5f) * minecraft->player->width * 0.9f;
    const float offsetY = (static_cast<float>((i >> 1) % 2) - 0.5f) * minecraft->player->height * 0.2f;
    const float offsetZ = (static_cast<float>((i >> 2) % 2) - 0.5f) * minecraft->player->width * 0.9f;
    const int sampleX = MathHelper::floor(static_cast<float>(blockX) + offsetX);
    const int sampleY = MathHelper::floor(static_cast<float>(blockY) + offsetY);
    const int sampleZ = MathHelper::floor(static_cast<float>(blockZ) + offsetZ);
    if(!minecraft->world->shouldSuffocate(sampleX, sampleY, sampleZ)) {
     continue;
    }
    blockId = minecraft->world->getBlockId(sampleX, sampleY, sampleZ);
   }
  }
  if(net::minecraft::block::Block* block =
         net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
     block != nullptr) {
   renderTexturedOverlay(block->getTexture(2));
  }
 }
 if(minecraft->player->isInFluid(net::minecraft::block::material::Material::WATER) &&
    (minecraft->gameRenderer == nullptr || minecraft->gameRenderer->frameSettings().underwaterOverlay)) {
  minecraft->textureManager.bindTexture(minecraft->textureManager.getTextureId("/misc/water.png"));
  renderUnderwaterOverlay(*minecraft->player, tickDelta);
 }
}
void HeldItemRenderer::tick() {
 if(minecraft == nullptr || minecraft->player == nullptr) {
  return;
 }
 prevHeight = height;
 auto* clientPlayer = dynamic_cast<net::minecraft::entity::player::ClientPlayerEntity*>(minecraft->player);
 if(clientPlayer == nullptr) {
  return;
 }
 ItemStack* selectedItem = clientPlayer->inventory.getSelectedItem();
 bool sameSelection = slot == clientPlayer->inventory.selectedSlot && selectedItem == stackSource_;
 if(stackSource_ == nullptr && selectedItem == nullptr) {
  sameSelection = true;
 }
 if(selectedItem != nullptr && stackSource_ != nullptr && selectedItem != stackSource_ &&
    selectedItem->itemId == stack.itemId && selectedItem->getDamage() == stack.getDamage()) {
  stack = *selectedItem;
  stackSource_ = selectedItem;
  sameSelection = true;
 }
 const float target = sameSelection ? 1.0f : 0.0f;
 float delta = target - height;
 constexpr float maxStep = 0.4f;
 if(delta < -maxStep) {
  delta = -maxStep;
 }
 if(delta > maxStep) {
  delta = maxStep;
 }
 height += delta;
 if(height < 0.1f) {
  stack = selectedItem != nullptr ? *selectedItem : ItemStack{};
  stackSource_ = selectedItem;
  slot = clientPlayer->inventory.selectedSlot;
 }
}
void HeldItemRenderer::place() {
 height = 0.0f;
}
void HeldItemRenderer::use() {
 height = 0.0f;
}
} // namespace net::minecraft::client::render::item
