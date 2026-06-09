#include "net/minecraft/client/render/item/HeldItemRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/item/ItemModelRenderer.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/entity/EntityRenderer.hpp"
#include "net/minecraft/client/render/entity/PlayerEntityRenderer.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/MapItem.hpp"
#include "net/minecraft/item/map/MapState.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"

#include <algorithm>
#include <cmath>

namespace net::minecraft::client::render::item {

namespace {

constexpr int kGlRescaleNormal = 32826;
constexpr int kGlBlend = 3042;
constexpr int kGlAlphaTest = 3008;
constexpr float kPi = 3.14159265358979323846f;

[[nodiscard]] float handSwingProgress(const net::minecraft::entity::LivingEntity& entity, float tickDelta)
{
    return entity.getHandSwingProgress(tickDelta);
}

void renderTexturedOverlay(int textureId)
{
    Tessellator& tessellator = Tessellator::INSTANCE;
    constexpr float brightness = 0.1f;
    gl::GL11::glColor4f(brightness, brightness, brightness, 0.5f);
    gl::GL11::glPushMatrix();
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
    gl::GL11::glPopMatrix();
    gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void renderUnderwaterOverlay(net::minecraft::entity::player::PlayerEntity& player, float tickDelta)
{
    Tessellator& tessellator = Tessellator::INSTANCE;
    const float brightness = player.getBrightnessAtEyes(tickDelta);
    gl::GL11::glColor4f(brightness, brightness, brightness, 0.5f);
    gl::GL11::glEnable(kGlBlend);
    gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
    gl::GL11::glPushMatrix();
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
    gl::GL11::glPopMatrix();
    gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    gl::GL11::glDisable(kGlBlend);
}

void renderFireOverlay()
{
    Tessellator& tessellator = Tessellator::INSTANCE;
    gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
    gl::GL11::glEnable(kGlBlend);
    gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
    constexpr float size = 1.0f;
    net::minecraft::block::Block* fireBlock = net::minecraft::block::Block::FIRE;
    for (int layer = 0; layer < 2; ++layer) {
        gl::GL11::glPushMatrix();
        const int texture = (fireBlock != nullptr ? fireBlock->textureId : 31) + layer * 16;
        const int uOrigin = (texture & 0xF) << 4;
        const int vOrigin = texture & 0xF0;
        const float uMin = static_cast<float>(uOrigin) / 256.0f;
        const float uMax = (static_cast<float>(uOrigin) + 15.99f) / 256.0f;
        const float vMin = static_cast<float>(vOrigin) / 256.0f;
        const float vMax = (static_cast<float>(vOrigin) + 15.99f) / 256.0f;
        const float x0 = (0.0f - size) / 2.0f;
        const float x1 = x0 + size;
        const float y0 = 0.0f - size / 2.0f;
        const float y1 = y0 + size;
        constexpr float depth = -0.5f;
        gl::GL11::glTranslatef(static_cast<float>(-(layer * 2 - 1)) * 0.24f, -0.3f, 0.0f);
        gl::GL11::glRotatef(static_cast<float>(layer * 2 - 1) * 10.0f, 0.0f, 1.0f, 0.0f);
        tessellator.startQuads();
        tessellator.vertex(x0, y0, depth, uMax, vMax);
        tessellator.vertex(x1, y0, depth, uMin, vMax);
        tessellator.vertex(x1, y1, depth, uMin, vMin);
        tessellator.vertex(x0, y1, depth, uMax, vMin);
        tessellator.draw();
        gl::GL11::glPopMatrix();
    }
    gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    gl::GL11::glDisable(kGlBlend);
}

} // namespace

HeldItemRenderer::HeldItemRenderer(net::minecraft::client::Minecraft* minecraftIn)
    : minecraft(minecraftIn),
      mapRenderer(minecraftIn != nullptr ? minecraftIn->textRenderer.get() : nullptr,
          minecraftIn != nullptr ? &minecraftIn->textureManager : nullptr)
{
}

void HeldItemRenderer::renderItem(const net::minecraft::entity::LivingEntity& entity, const ItemStack& itemStack)
{
    if (minecraft == nullptr || itemStack.empty()) {
        return;
    }

    gl::GL11::glPushMatrix();
    if (ItemModelRenderer::rendersAsBlockModel(itemStack)) {
        net::minecraft::block::Block* block = ItemModelRenderer::blockOf(itemStack);
        gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, minecraft->textureManager.getTextureId("/terrain.png"));
        blockRenderManager.render(*block, itemStack.getDamage(), entity.getBrightnessAtEyes(1.0f));
        gl::GL11::glPopMatrix();
        return;
    }

    gl::GL11::glBindTexture(
        gl::GL11::GL_TEXTURE_2D, minecraft->textureManager.getTextureId(ItemModelRenderer::spriteAtlasPath(itemStack)));

    Tessellator& tessellator = Tessellator::INSTANCE;
    const int textureIndex = entity.getItemStackTextureId(itemStack);
    const float uMax = (static_cast<float>(textureIndex % 16 * 16) + 0.0f) / 256.0f;
    const float uMin = (static_cast<float>(textureIndex % 16 * 16) + 15.99f) / 256.0f;
    const float vMax = (static_cast<float>(textureIndex / 16 * 16) + 0.0f) / 256.0f;
    const float vMin = (static_cast<float>(textureIndex / 16 * 16) + 15.99f) / 256.0f;
    constexpr float itemWidth = 1.0f;
    gl::GL11::glEnable(kGlRescaleNormal);
    gl::GL11::glTranslatef(0.0f, -0.3f, 0.0f);
    constexpr float itemScale = 1.5f;
    gl::GL11::glScalef(itemScale, itemScale, itemScale);
    gl::GL11::glRotatef(50.0f, 0.0f, 1.0f, 0.0f);
    gl::GL11::glRotatef(335.0f, 0.0f, 0.0f, 1.0f);
    gl::GL11::glTranslatef(-0.9375f, -0.0625f, 0.0f);
    constexpr float depth = 0.0625f;

    tessellator.startQuads();
    tessellator.normal(0.0f, 0.0f, 1.0f);
    tessellator.vertex(0.0, 0.0, 0.0, uMin, vMin);
    tessellator.vertex(itemWidth, 0.0, 0.0, uMax, vMin);
    tessellator.vertex(itemWidth, 1.0, 0.0, uMax, vMax);
    tessellator.vertex(0.0, 1.0, 0.0, uMin, vMax);
    tessellator.draw();

    tessellator.startQuads();
    tessellator.normal(0.0f, 0.0f, -1.0f);
    tessellator.vertex(0.0, 1.0, 0.0 - depth, uMin, vMax);
    tessellator.vertex(itemWidth, 1.0, 0.0 - depth, uMax, vMax);
    tessellator.vertex(itemWidth, 0.0, 0.0 - depth, uMax, vMin);
    tessellator.vertex(0.0, 0.0, 0.0 - depth, uMin, vMin);
    tessellator.draw();

    tessellator.startQuads();
    tessellator.normal(-1.0f, 0.0f, 0.0f);
    for (int slice = 0; slice < 16; ++slice) {
        const float t = static_cast<float>(slice) / 16.0f;
        const float u = uMin + (uMax - uMin) * t - 0.001953125f;
        const float x = itemWidth * t;
        tessellator.vertex(x, 0.0, 0.0 - depth, u, vMin);
        tessellator.vertex(x, 0.0, 0.0, u, vMin);
        tessellator.vertex(x, 1.0, 0.0, u, vMax);
        tessellator.vertex(x, 1.0, 0.0 - depth, u, vMax);
    }
    tessellator.draw();

    tessellator.startQuads();
    tessellator.normal(1.0f, 0.0f, 0.0f);
    for (int slice = 0; slice < 16; ++slice) {
        const float t = static_cast<float>(slice) / 16.0f;
        const float u = uMin + (uMax - uMin) * t - 0.001953125f;
        const float x = itemWidth * t + 0.0625f;
        tessellator.vertex(x, 1.0, 0.0 - depth, u, vMax);
        tessellator.vertex(x, 1.0, 0.0, u, vMax);
        tessellator.vertex(x, 0.0, 0.0, u, vMin);
        tessellator.vertex(x, 0.0, 0.0 - depth, u, vMin);
    }
    tessellator.draw();

    tessellator.startQuads();
    tessellator.normal(0.0f, 1.0f, 0.0f);
    for (int slice = 0; slice < 16; ++slice) {
        const float t = static_cast<float>(slice) / 16.0f;
        const float v = vMin + (vMax - vMin) * t - 0.001953125f;
        const float y = itemWidth * t + 0.0625f;
        tessellator.vertex(0.0, y, 0.0, uMin, v);
        tessellator.vertex(itemWidth, y, 0.0, uMax, v);
        tessellator.vertex(itemWidth, y, 0.0 - depth, uMax, v);
        tessellator.vertex(0.0, y, 0.0 - depth, uMin, v);
    }
    tessellator.draw();

    tessellator.startQuads();
    tessellator.normal(0.0f, -1.0f, 0.0f);
    for (int slice = 0; slice < 16; ++slice) {
        const float t = static_cast<float>(slice) / 16.0f;
        const float v = vMin + (vMax - vMin) * t - 0.001953125f;
        const float y = itemWidth * t;
        tessellator.vertex(itemWidth, y, 0.0, uMax, v);
        tessellator.vertex(0.0, y, 0.0, uMin, v);
        tessellator.vertex(0.0, y, 0.0 - depth, uMin, v);
        tessellator.vertex(itemWidth, y, 0.0 - depth, uMax, v);
    }
    tessellator.draw();
    gl::GL11::glDisable(kGlRescaleNormal);
    gl::GL11::glPopMatrix();
}

void HeldItemRenderer::render(float tickDelta)
{
    if (minecraft == nullptr || minecraft->player == nullptr || minecraft->world == nullptr) {
        return;
    }

    auto* clientPlayer = dynamic_cast<net::minecraft::entity::player::ClientPlayerEntity*>(minecraft->player);
    if (clientPlayer == nullptr) {
        return;
    }

    const float equipProgress = prevHeight + (height - prevHeight) * tickDelta;
    const float pitch = clientPlayer->prevPitch + (clientPlayer->pitch - clientPlayer->prevPitch) * tickDelta;
    gl::GL11::glPushMatrix();
    gl::GL11::glRotatef(pitch, 1.0f, 0.0f, 0.0f);
    gl::GL11::glRotatef(clientPlayer->prevYaw + (clientPlayer->yaw - clientPlayer->prevYaw) * tickDelta, 0.0f, 1.0f, 0.0f);
    platform::Lighting::turnOn();
    gl::GL11::glPopMatrix();

    const ItemStack* selectedItem = stack.empty() ? nullptr : &stack;
    const float brightness = minecraft->world->getLightBrightness(MathHelper::floor(clientPlayer->x),
        MathHelper::floor(clientPlayer->y), MathHelper::floor(clientPlayer->z));
    if (selectedItem != nullptr && selectedItem->getItem() != nullptr) {
        const int tint = selectedItem->getItem()->getColorMultiplier(selectedItem->getDamage());
        const float red = static_cast<float>((tint >> 16) & 0xFF) / 255.0f;
        const float green = static_cast<float>((tint >> 8) & 0xFF) / 255.0f;
        const float blue = static_cast<float>(tint & 0xFF) / 255.0f;
        gl::GL11::glColor4f(brightness * red, brightness * green, brightness * blue, 1.0f);
    } else {
        gl::GL11::glColor4f(brightness, brightness, brightness, 1.0f);
    }

    entity::EntityRenderer* entityRenderer = entity::EntityRenderDispatcher::instance().get(*clientPlayer);
    auto* playerRenderer = dynamic_cast<entity::PlayerEntityRenderer*>(entityRenderer);

    if (selectedItem != nullptr && Item::MAP != nullptr && selectedItem->itemId == Item::MAP->id) {
        gl::GL11::glPushMatrix();
        constexpr float scale = 0.8f;
        float swing = handSwingProgress(*clientPlayer, tickDelta);
        float swingSin = MathHelper::sin(swing * static_cast<float>(kPi));
        float swingSqrt = MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi));
        gl::GL11::glTranslatef(-swingSqrt * 0.4f, MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi) * 2.0f) * 0.2f,
            -swingSin * 0.2f);
        float pitchFactor = 1.0f - pitch / 45.0f + 0.1f;
        pitchFactor = std::clamp(pitchFactor, 0.0f, 1.0f);
        pitchFactor = -MathHelper::cos(pitchFactor * static_cast<float>(kPi)) * 0.5f + 0.5f;
        gl::GL11::glTranslatef(0.0f, 0.0f * scale - (1.0f - equipProgress) * 1.2f - pitchFactor * 0.5f + 0.04f, -0.9f * scale);
        gl::GL11::glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        gl::GL11::glRotatef(pitchFactor * -85.0f, 0.0f, 0.0f, 1.0f);
        gl::GL11::glEnable(kGlRescaleNormal);
        const int skinTexture = minecraft->textureManager.downloadTexture(clientPlayer->skinUrl, clientPlayer->getTexture());
        gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, skinTexture);
        if (playerRenderer != nullptr) {
            for (int side = 0; side < 2; ++side) {
                const int mirror = side * 2 - 1;
                gl::GL11::glPushMatrix();
                gl::GL11::glTranslatef(0.0f, -0.6f, 1.1f * static_cast<float>(mirror));
                gl::GL11::glRotatef(static_cast<float>(-45 * mirror), 1.0f, 0.0f, 0.0f);
                gl::GL11::glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
                gl::GL11::glRotatef(59.0f, 0.0f, 0.0f, 1.0f);
                gl::GL11::glRotatef(static_cast<float>(-65 * mirror), 0.0f, 1.0f, 0.0f);
                playerRenderer->renderHand();
                gl::GL11::glPopMatrix();
            }
        }
        swing = handSwingProgress(*clientPlayer, tickDelta);
        const float swingSin2 = MathHelper::sin(swing * swing * static_cast<float>(kPi));
        swingSqrt = MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi));
        gl::GL11::glRotatef(-swingSin2 * 20.0f, 0.0f, 1.0f, 0.0f);
        gl::GL11::glRotatef(-swingSqrt * 20.0f, 0.0f, 0.0f, 1.0f);
        gl::GL11::glRotatef(-swingSqrt * 80.0f, 1.0f, 0.0f, 0.0f);
        gl::GL11::glScalef(0.38f, 0.38f, 0.38f);
        gl::GL11::glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        gl::GL11::glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
        gl::GL11::glTranslatef(-1.0f, -1.0f, 0.0f);
        gl::GL11::glScalef(0.015625f, 0.015625f, 0.015625f);
        gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, minecraft->textureManager.getTextureId("/misc/mapbg.png"));
        Tessellator& tessellator = Tessellator::INSTANCE;
        gl::GL11::glNormal3f(0.0f, 0.0f, -1.0f);
        tessellator.startQuads();
        constexpr int border = 7;
        tessellator.vertex(0 - border, 128 + border, 0.0, 0.0, 1.0);
        tessellator.vertex(128 + border, 128 + border, 0.0, 1.0, 1.0);
        tessellator.vertex(128 + border, 0 - border, 0.0, 1.0, 0.0);
        tessellator.vertex(0 - border, 0 - border, 0.0, 0.0, 0.0);
        tessellator.draw();
        if (Item::MAP != nullptr) {
            ItemStack mapStack = *selectedItem;
            if (auto* mapItem = dynamic_cast<::net::minecraft::item::MapItem*>(Item::MAP)) {
                if (map::MapState* mapState = mapItem->getSavedMapState(mapStack, minecraft->world)) {
                    mapRenderer.render(*clientPlayer, minecraft->textureManager, *mapState);
                }
            }
        }
        gl::GL11::glPopMatrix();
    } else if (selectedItem != nullptr) {
        gl::GL11::glPushMatrix();
        constexpr float scale = 0.8f;
        float swing = handSwingProgress(*clientPlayer, tickDelta);
        float swingSin = MathHelper::sin(swing * static_cast<float>(kPi));
        float swingSqrt = MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi));
        gl::GL11::glTranslatef(-swingSqrt * 0.4f, MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi) * 2.0f) * 0.2f,
            -swingSin * 0.2f);
        gl::GL11::glTranslatef(0.7f * scale, -0.65f * scale - (1.0f - equipProgress) * 0.6f, -0.9f * scale);
        gl::GL11::glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
        gl::GL11::glEnable(kGlRescaleNormal);
        swing = handSwingProgress(*clientPlayer, tickDelta);
        swingSin = MathHelper::sin(swing * swing * static_cast<float>(kPi));
        swingSqrt = MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi));
        gl::GL11::glRotatef(-swingSin * 20.0f, 0.0f, 1.0f, 0.0f);
        gl::GL11::glRotatef(-swingSqrt * 20.0f, 0.0f, 0.0f, 1.0f);
        gl::GL11::glRotatef(-swingSqrt * 80.0f, 1.0f, 0.0f, 0.0f);
        gl::GL11::glScalef(0.4f, 0.4f, 0.4f);
        if (selectedItem->getItem() != nullptr && selectedItem->getItem()->isHandheldRod()) {
            gl::GL11::glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
        }
        renderItem(*clientPlayer, *selectedItem);
        gl::GL11::glPopMatrix();
    } else if (playerRenderer != nullptr) {
        gl::GL11::glPushMatrix();
        constexpr float scale = 0.8f;
        float swing = handSwingProgress(*clientPlayer, tickDelta);
        float swingSin = MathHelper::sin(swing * static_cast<float>(kPi));
        float swingSqrt = MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi));
        gl::GL11::glTranslatef(-swingSqrt * 0.3f, MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi) * 2.0f) * 0.4f,
            -swingSin * 0.4f);
        gl::GL11::glTranslatef(0.8f * scale, -0.75f * scale - (1.0f - equipProgress) * 0.6f, -0.9f * scale);
        gl::GL11::glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
        gl::GL11::glEnable(kGlRescaleNormal);
        swing = handSwingProgress(*clientPlayer, tickDelta);
        swingSin = MathHelper::sin(swing * swing * static_cast<float>(kPi));
        swingSqrt = MathHelper::sin(MathHelper::sqrt(swing) * static_cast<float>(kPi));
        gl::GL11::glRotatef(swingSqrt * 70.0f, 0.0f, 1.0f, 0.0f);
        gl::GL11::glRotatef(-swingSin * 20.0f, 0.0f, 0.0f, 1.0f);
        const int skinTexture = minecraft->textureManager.downloadTexture(clientPlayer->skinUrl, clientPlayer->getTexture());
        gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, skinTexture);
        gl::GL11::glTranslatef(-1.0f, 3.6f, 3.5f);
        gl::GL11::glRotatef(120.0f, 0.0f, 0.0f, 1.0f);
        gl::GL11::glRotatef(200.0f, 1.0f, 0.0f, 0.0f);
        gl::GL11::glRotatef(-135.0f, 0.0f, 1.0f, 0.0f);
        gl::GL11::glTranslatef(5.6f, 0.0f, 0.0f);
        playerRenderer->renderHand();
        gl::GL11::glPopMatrix();
    }

    gl::GL11::glDisable(kGlRescaleNormal);
    platform::Lighting::turnOff();
}

void HeldItemRenderer::renderScreenOverlays(float tickDelta)
{
    if (minecraft == nullptr || minecraft->player == nullptr || minecraft->world == nullptr) {
        return;
    }

    gl::GL11::glDisable(kGlAlphaTest);
    if (minecraft->player->isOnFire()) {
        gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, minecraft->textureManager.getTextureId("/terrain.png"));
        renderFireOverlay();
    }
    if (minecraft->player->isInsideWall()) {
        const int blockX = MathHelper::floor(minecraft->player->x);
        const int blockY = MathHelper::floor(minecraft->player->y);
        const int blockZ = MathHelper::floor(minecraft->player->z);
        gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, minecraft->textureManager.getTextureId("/terrain.png"));
        int blockId = minecraft->world->getBlockId(blockX, blockY, blockZ);
        if (minecraft->world->shouldSuffocate(blockX, blockY, blockZ)) {
            if (net::minecraft::block::Block* block = net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
                block != nullptr) {
                renderTexturedOverlay(block->getTexture(2));
            }
        } else {
            for (int i = 0; i < 8; ++i) {
                const float offsetX = (static_cast<float>((i >> 0) % 2) - 0.5f) * minecraft->player->width * 0.9f;
                const float offsetY = (static_cast<float>((i >> 1) % 2) - 0.5f) * minecraft->player->height * 0.2f;
                const float offsetZ = (static_cast<float>((i >> 2) % 2) - 0.5f) * minecraft->player->width * 0.9f;
                const int sampleX = MathHelper::floor(static_cast<float>(blockX) + offsetX);
                const int sampleY = MathHelper::floor(static_cast<float>(blockY) + offsetY);
                const int sampleZ = MathHelper::floor(static_cast<float>(blockZ) + offsetZ);
                if (!minecraft->world->shouldSuffocate(sampleX, sampleY, sampleZ)) {
                    continue;
                }
                blockId = minecraft->world->getBlockId(sampleX, sampleY, sampleZ);
            }
        }
        if (net::minecraft::block::Block* block = net::minecraft::block::Block::BLOCKS[static_cast<std::size_t>(blockId)];
            block != nullptr) {
            renderTexturedOverlay(block->getTexture(2));
        }
    }
    if (minecraft->player->isInFluid(net::minecraft::block::material::Material::WATER)) {
        gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, minecraft->textureManager.getTextureId("/misc/water.png"));
        renderUnderwaterOverlay(*minecraft->player, tickDelta);
    }
    gl::GL11::glEnable(kGlAlphaTest);
}

void HeldItemRenderer::tick()
{
    if (minecraft == nullptr || minecraft->player == nullptr) {
        return;
    }

    prevHeight = height;
    auto* clientPlayer = dynamic_cast<net::minecraft::entity::player::ClientPlayerEntity*>(minecraft->player);
    if (clientPlayer == nullptr) {
        return;
    }

    ItemStack* selectedItem = clientPlayer->inventory.getSelectedItem();
    bool sameSelection = slot == clientPlayer->inventory.selectedSlot && selectedItem == stackSource_;
    if (stackSource_ == nullptr && selectedItem == nullptr) {
        sameSelection = true;
    }
    if (selectedItem != nullptr && stackSource_ != nullptr && selectedItem != stackSource_
        && selectedItem->itemId == stack.itemId && selectedItem->getDamage() == stack.getDamage()) {
        stack = *selectedItem;
        stackSource_ = selectedItem;
        sameSelection = true;
    }

    const float target = sameSelection ? 1.0f : 0.0f;
    float delta = target - height;
    constexpr float maxStep = 0.4f;
    if (delta < -maxStep) {
        delta = -maxStep;
    }
    if (delta > maxStep) {
        delta = maxStep;
    }
    height += delta;
    if (height < 0.1f) {
        stack = selectedItem != nullptr ? *selectedItem : ItemStack {};
        stackSource_ = selectedItem;
        slot = clientPlayer->inventory.selectedSlot;
    }
}

void HeldItemRenderer::place()
{
    height = 0.0f;
}

void HeldItemRenderer::use()
{
    height = 0.0f;
}

} // namespace net::minecraft::client::render::item
