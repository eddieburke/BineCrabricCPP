#include "net/minecraft/client/render/item/HeldItemRenderer.hpp"
#include <algorithm>
#include <cmath>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/entity/EntityRenderer.hpp"
#include "net/minecraft/client/render/entity/PlayerEntityRenderer.hpp"
#include "net/minecraft/client/render/item/ItemModelRenderer.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/MapItem.hpp"
#include "net/minecraft/item/map/MapState.hpp"
#include "net/minecraft/mod/ModTexture.hpp"
#include "net/minecraft/mod/model/ModelDraw.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::client::render::item {
namespace {
constexpr float kPi = 3.14159265358979323846f;
[[nodiscard]] float handSwingProgress(const net::minecraft::entity::LivingEntity& entity, float tickDelta) {
  return entity.getHandSwingProgress(tickDelta);
}
void renderTexturedOverlay(int textureId) {
  Tessellator& tessellator = Tessellator::INSTANCE;
  constexpr float brightness = 0.1f;
  gl::color4f(brightness, brightness, brightness, 0.5f);
  gl::pushMatrix();
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
  gl::popMatrix();
  gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
}
void renderUnderwaterOverlay(net::minecraft::entity::player::PlayerEntity& player, float tickDelta) {
  Tessellator& tessellator = Tessellator::INSTANCE;
  const float brightness = player.getBrightnessAtEyes(tickDelta);
  gl::color4f(brightness, brightness, brightness, 0.5f);
  const gl::preset::HeldItemUnderwater underwaterCaps;
  gl::MatrixGuard underwaterMatrix;
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
  gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
}
void renderFireOverlay() {
  const gl::preset::FireOverlay fireCaps;
  Tessellator& tessellator = Tessellator::INSTANCE;
  gl::color4f(1.0f, 1.0f, 1.0f, 0.9f);
  constexpr float size = 1.0f;
  net::minecraft::block::Block* fireBlock = net::minecraft::block::Block::FIRE;
  const float x0 = (0.0f - size) / 2.0f;
  const float x1 = x0 + size;
  const float y0 = 0.0f - size / 2.0f;
  const float y1 = y0 + size;
  constexpr float depth = -0.5f;
  float baseModelView[16]{};
  gl::getFloatv(gl::matrix_::ModelViewMatrix, baseModelView);
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
    model.set(baseModelView);
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
  gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
}
} // namespace
HeldItemRenderer::HeldItemRenderer(net::minecraft::client::Minecraft* minecraftIn)
    : minecraft(minecraftIn),
      mapRenderer(minecraftIn != nullptr ? minecraftIn->textRenderer.get() : nullptr,
                  minecraftIn != nullptr ? &minecraftIn->textureManager : nullptr) {
}
void HeldItemRenderer::renderItem(const net::minecraft::entity::LivingEntity& entity, const ItemStack& itemStack) {
  if(minecraft == nullptr || itemStack.empty()) {
    return;
  }
  const gl::MatrixGuard matrix;
  if(ItemModelRenderer::hasCustomModel(itemStack)) {
    if(ItemModelRenderer::usesModTexture(itemStack)) {
      net::minecraft::mod::bind(minecraft->textureManager, itemStack.getTextureId());
    } else {
      gl::bindTexture(gl::cap::Texture2D,
                      minecraft->textureManager.getTextureId(ItemModelRenderer::spriteAtlasPath(itemStack)));
    }
    gl::translatef(0.0f, -0.3f, 0.0f);
    gl::scalef(1.5f, 1.5f, 1.5f);
    gl::rotatef(50.0f, 0.0f, 1.0f, 0.0f);
    gl::rotatef(335.0f, 0.0f, 0.0f, 1.0f);
    gl::translatef(-0.5f, -0.5f, -0.5f);
    net::minecraft::mod::model::drawLuaItemModel(Tessellator::INSTANCE, itemStack, entity.getBrightnessAtEyes(1.0f));
    return;
  }
  if(ItemModelRenderer::rendersAsBlockModel(itemStack)) {
    net::minecraft::block::Block* block = ItemModelRenderer::blockOf(itemStack);
    gl::bindTexture(gl::cap::Texture2D, minecraft->textureManager.getTextureId("/terrain.png"));
    blockRenderManager.ctx.textureManager = &minecraft->textureManager;
    blockRenderManager.render(*block, itemStack.getDamage(), entity.getBrightnessAtEyes(1.0f));
    blockRenderManager.ctx.textureManager = nullptr;
    return;
  }
  if(ItemModelRenderer::usesModTexture(itemStack)) {
    net::minecraft::mod::bind(minecraft->textureManager, itemStack.getTextureId());
  } else {
    gl::bindTexture(gl::cap::Texture2D,
                    minecraft->textureManager.getTextureId(ItemModelRenderer::spriteAtlasPath(itemStack)));
  }
  Tessellator& tessellator = Tessellator::INSTANCE;
  const auto uv = ItemModelRenderer::spriteUv(itemStack);
  const float uMin = static_cast<float>(uv.uMin);
  const float uMax = static_cast<float>(uv.uMax);
  const float vMin = static_cast<float>(uv.vMin);
  const float vMax = static_cast<float>(uv.vMax);
  constexpr float itemWidth = 1.0f;
  gl::translatef(0.0f, -0.3f, 0.0f);
  constexpr float itemScale = 1.5f;
  gl::scalef(itemScale, itemScale, itemScale);
  gl::rotatef(50.0f, 0.0f, 1.0f, 0.0f);
  gl::rotatef(335.0f, 0.0f, 0.0f, 1.0f);
  gl::translatef(-0.9375f, -0.0625f, 0.0f);
  constexpr float depth = 0.0625f;
  tessellator.startQuads();
  tessellator.normal(0.0f, 0.0f, 1.0f);
  tessellator.vertex(0.0, 0.0, 0.0, uMax, vMax);
  tessellator.vertex(itemWidth, 0.0, 0.0, uMin, vMax);
  tessellator.vertex(itemWidth, 1.0, 0.0, uMin, vMin);
  tessellator.vertex(0.0, 1.0, 0.0, uMax, vMin);
  tessellator.normal(0.0f, 0.0f, -1.0f);
  tessellator.vertex(0.0, 1.0, 0.0 - depth, uMax, vMin);
  tessellator.vertex(itemWidth, 1.0, 0.0 - depth, uMin, vMin);
  tessellator.vertex(itemWidth, 0.0, 0.0 - depth, uMin, vMax);
  tessellator.vertex(0.0, 0.0, 0.0 - depth, uMax, vMax);
  tessellator.normal(-1.0f, 0.0f, 0.0f);
  for(int slice = 0; slice < 16; ++slice) {
    const float t = static_cast<float>(slice) / 16.0f;
    const float u = uMax + (uMin - uMax) * t - 0.001953125f;
    const float x = itemWidth * t;
    tessellator.vertex(x, 0.0, 0.0 - depth, u, vMax);
    tessellator.vertex(x, 0.0, 0.0, u, vMax);
    tessellator.vertex(x, 1.0, 0.0, u, vMin);
    tessellator.vertex(x, 1.0, 0.0 - depth, u, vMin);
  }
  tessellator.normal(1.0f, 0.0f, 0.0f);
  for(int slice = 0; slice < 16; ++slice) {
    const float t = static_cast<float>(slice) / 16.0f;
    const float u = uMax + (uMin - uMax) * t - 0.001953125f;
    const float x = itemWidth * t + 0.0625f;
    tessellator.vertex(x, 1.0, 0.0 - depth, u, vMin);
    tessellator.vertex(x, 1.0, 0.0, u, vMin);
    tessellator.vertex(x, 0.0, 0.0, u, vMax);
    tessellator.vertex(x, 0.0, 0.0 - depth, u, vMax);
  }
  tessellator.normal(0.0f, 1.0f, 0.0f);
  for(int slice = 0; slice < 16; ++slice) {
    const float t = static_cast<float>(slice) / 16.0f;
    const float v = vMax + (vMin - vMax) * t - 0.001953125f;
    const float y = itemWidth * t + 0.0625f;
    tessellator.vertex(0.0, y, 0.0, uMax, v);
    tessellator.vertex(itemWidth, y, 0.0, uMin, v);
    tessellator.vertex(itemWidth, y, 0.0 - depth, uMin, v);
    tessellator.vertex(0.0, y, 0.0 - depth, uMax, v);
  }
  tessellator.normal(0.0f, -1.0f, 0.0f);
  for(int slice = 0; slice < 16; ++slice) {
    const float t = static_cast<float>(slice) / 16.0f;
    const float v = vMax + (vMin - vMax) * t - 0.001953125f;
    const float y = itemWidth * t;
    tessellator.vertex(itemWidth, y, 0.0, uMin, v);
    tessellator.vertex(0.0, y, 0.0, uMax, v);
    tessellator.vertex(0.0, y, 0.0 - depth, uMax, v);
    tessellator.vertex(itemWidth, y, 0.0 - depth, uMin, v);
  }
  tessellator.draw();
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
  gl::pushMatrix();
  gl::rotatef(pitch, 1.0f, 0.0f, 0.0f);
  gl::rotatef(clientPlayer->prevYaw + (clientPlayer->yaw - clientPlayer->prevYaw) * tickDelta, 0.0f, 1.0f, 0.0f);
  platform::Lighting::turnOn();
  gl::popMatrix();
  const ItemStack* selectedItem = stack.empty() ? nullptr : &stack;
  const float brightness = minecraft->world->getLightBrightness(
      MathHelper::floor(clientPlayer->x), MathHelper::floor(clientPlayer->y), MathHelper::floor(clientPlayer->z));
  if(selectedItem != nullptr && selectedItem->getItem() != nullptr) {
    const int tint = selectedItem->getItem()->getColorMultiplier(selectedItem->getDamage());
    const float red = static_cast<float>((tint >> 16) & 0xFF) / 255.0f;
    const float green = static_cast<float>((tint >> 8) & 0xFF) / 255.0f;
    const float blue = static_cast<float>(tint & 0xFF) / 255.0f;
    gl::color4f(brightness * red, brightness * green, brightness * blue, 1.0f);
  } else {
    gl::color4f(brightness, brightness, brightness, 1.0f);
  }
  entity::EntityRenderer* entityRenderer = entity::EntityRenderDispatcher::instance().get(*clientPlayer);
  auto* playerRenderer = dynamic_cast<entity::PlayerEntityRenderer*>(entityRenderer);
  const gl::preset::HeldItemFirstPerson firstPersonCaps;
  if(selectedItem != nullptr && Item::byRawId(102) != nullptr && selectedItem->itemId == Item::byRawId(102)->id) {
    gl::pushMatrix();
    constexpr float scale = 0.8f;
    float swing = handSwingProgress(*clientPlayer, tickDelta);
    float swingSin = MathHelper::sin(swing * static_cast<float>(kPi));
    float swingSqrt = MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi));
    gl::translatef(-swingSqrt * 0.4f,
                   MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi) * 2.0f) * 0.2f,
                   -swingSin * 0.2f);
    float pitchFactor = 1.0f - pitch / 45.0f + 0.1f;
    pitchFactor = std::clamp(pitchFactor, 0.0f, 1.0f);
    pitchFactor = -MathHelper::cos(pitchFactor * static_cast<float>(kPi)) * 0.5f + 0.5f;
    gl::translatef(0.0f, 0.0f * scale - (1.0f - equipProgress) * 1.2f - pitchFactor * 0.5f + 0.04f, -0.9f * scale);
    gl::rotatef(90.0f, 0.0f, 1.0f, 0.0f);
    gl::rotatef(pitchFactor * -85.0f, 0.0f, 0.0f, 1.0f);
    const int skinTexture =
        minecraft->textureManager.downloadTexture(clientPlayer->skinUrl, clientPlayer->getTexture());
    gl::bindTexture(gl::cap::Texture2D, skinTexture);
    if(playerRenderer != nullptr) {
      for(int side = 0; side < 2; ++side) {
        const int mirror = side * 2 - 1;
        gl::pushMatrix();
        gl::translatef(0.0f, -0.6f, 1.1f * static_cast<float>(mirror));
        gl::rotatef(static_cast<float>(-45 * mirror), 1.0f, 0.0f, 0.0f);
        gl::rotatef(-90.0f, 0.0f, 0.0f, 1.0f);
        gl::rotatef(59.0f, 0.0f, 0.0f, 1.0f);
        gl::rotatef(static_cast<float>(-65 * mirror), 0.0f, 1.0f, 0.0f);
        playerRenderer->renderHand();
        gl::popMatrix();
      }
    }
    swing = handSwingProgress(*clientPlayer, tickDelta);
    const float swingSin2 = MathHelper::sin(swing * swing * static_cast<float>(kPi));
    swingSqrt = MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi));
    gl::rotatef(-swingSin2 * 20.0f, 0.0f, 1.0f, 0.0f);
    gl::rotatef(-swingSqrt * 20.0f, 0.0f, 0.0f, 1.0f);
    gl::rotatef(-swingSqrt * 80.0f, 1.0f, 0.0f, 0.0f);
    gl::scalef(0.38f, 0.38f, 0.38f);
    gl::rotatef(90.0f, 0.0f, 1.0f, 0.0f);
    gl::rotatef(180.0f, 0.0f, 0.0f, 1.0f);
    gl::translatef(-1.0f, -1.0f, 0.0f);
    gl::scalef(0.015625f, 0.015625f, 0.015625f);
    gl::bindTexture(gl::cap::Texture2D, minecraft->textureManager.getTextureId("/misc/mapbg.png"));
    Tessellator& tessellator = Tessellator::INSTANCE;
    gl::normal3f(0.0f, 0.0f, -1.0f);
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
    gl::popMatrix();
  } else if(selectedItem != nullptr) {
    gl::pushMatrix();
    constexpr float scale = 0.8f;
    float swing = handSwingProgress(*clientPlayer, tickDelta);
    float swingSin = MathHelper::sin(swing * static_cast<float>(kPi));
    float swingSqrt = MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi));
    gl::translatef(-swingSqrt * 0.4f,
                   MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi) * 2.0f) * 0.2f,
                   -swingSin * 0.2f);
    gl::translatef(0.7f * scale, -0.65f * scale - (1.0f - equipProgress) * 0.6f, -0.9f * scale);
    gl::rotatef(45.0f, 0.0f, 1.0f, 0.0f);
    swing = handSwingProgress(*clientPlayer, tickDelta);
    swingSin = MathHelper::sin(swing * swing * static_cast<float>(kPi));
    swingSqrt = MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi));
    gl::rotatef(-swingSin * 20.0f, 0.0f, 1.0f, 0.0f);
    gl::rotatef(-swingSqrt * 20.0f, 0.0f, 0.0f, 1.0f);
    gl::rotatef(-swingSqrt * 80.0f, 1.0f, 0.0f, 0.0f);
    gl::scalef(0.4f, 0.4f, 0.4f);
    if(selectedItem->getItem() != nullptr && selectedItem->getItem()->isHandheldRod()) {
      gl::rotatef(180.0f, 0.0f, 1.0f, 0.0f);
    }
    renderItem(*clientPlayer, *selectedItem);
    gl::popMatrix();
  } else if(playerRenderer != nullptr) {
    gl::pushMatrix();
    constexpr float scale = 0.8f;
    float swing = handSwingProgress(*clientPlayer, tickDelta);
    float swingSin = MathHelper::sin(swing * static_cast<float>(kPi));
    float swingSqrt = MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi));
    gl::translatef(-swingSqrt * 0.3f,
                   MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi) * 2.0f) * 0.4f,
                   -swingSin * 0.4f);
    gl::translatef(0.8f * scale, -0.75f * scale - (1.0f - equipProgress) * 0.6f, -0.9f * scale);
    gl::rotatef(45.0f, 0.0f, 1.0f, 0.0f);
    swing = handSwingProgress(*clientPlayer, tickDelta);
    swingSin = MathHelper::sin(swing * swing * static_cast<float>(kPi));
    swingSqrt = MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi));
    gl::rotatef(swingSqrt * 70.0f, 0.0f, 1.0f, 0.0f);
    gl::rotatef(-swingSin * 20.0f, 0.0f, 0.0f, 1.0f);
    const int skinTexture =
        minecraft->textureManager.downloadTexture(clientPlayer->skinUrl, clientPlayer->getTexture());
    gl::bindTexture(gl::cap::Texture2D, skinTexture);
    gl::translatef(-1.0f, 3.6f, 3.5f);
    gl::rotatef(120.0f, 0.0f, 0.0f, 1.0f);
    gl::rotatef(200.0f, 1.0f, 0.0f, 0.0f);
    gl::rotatef(-135.0f, 0.0f, 1.0f, 0.0f);
    gl::translatef(5.6f, 0.0f, 0.0f);
    playerRenderer->renderHand();
    gl::popMatrix();
  }
  platform::Lighting::turnOff();
}
void HeldItemRenderer::renderScreenOverlays(float tickDelta) {
  if(minecraft == nullptr || minecraft->player == nullptr || minecraft->world == nullptr) {
    return;
  }
  const gl::preset::BlockOverlay overlayCaps;
  if(minecraft->player->isOnFire()) {
    gl::bindTexture(gl::cap::Texture2D, minecraft->textureManager.getTextureId("/terrain.png"));
    renderFireOverlay();
  }
  if(minecraft->player->isInsideWall()) {
    const int blockX = MathHelper::floor(minecraft->player->x);
    const int blockY = MathHelper::floor(minecraft->player->y);
    const int blockZ = MathHelper::floor(minecraft->player->z);
    gl::bindTexture(gl::cap::Texture2D, minecraft->textureManager.getTextureId("/terrain.png"));
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
  if(minecraft->player->isInFluid(net::minecraft::block::material::Material::WATER)) {
    gl::bindTexture(gl::cap::Texture2D, minecraft->textureManager.getTextureId("/misc/water.png"));
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
