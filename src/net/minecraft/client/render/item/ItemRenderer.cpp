#include "net/minecraft/client/render/item/ItemRenderer.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/render/item/ItemModelRenderer.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
namespace net::minecraft::client::render::item {
void ItemRenderer::renderGuiItem(client::font::TextRenderer& textRenderer,
                                 client::texture::TextureManager& textureManager, const ItemStack& stack, int x, int y) {
  (void)textRenderer;
  if(stack.count <= 0 || stack.itemId <= 0) {
    return;
  }
  if(ItemModelRenderer::rendersAsBlockModel(stack)) {
    renderBlockItemInGui(textureManager, stack, x, y);
  } else {
    renderSpriteItemInGui(textureManager, stack, x, y);
  }
  gl::GL11::glEnable(gl::GL11::GL_CULL_FACE);
}
void ItemRenderer::renderBlockItemInGui(client::texture::TextureManager& textureManager, const ItemStack& stack, int x,
                                        int y) {
  Block* block = ItemModelRenderer::blockOf(stack);
  textureManager.bindTexture(textureManager.getTextureId("/terrain.png"));
  gl::GL11::glPushMatrix();
  gl::GL11::glTranslatef(static_cast<float>(x - 2), static_cast<float>(y + 3), -3.0f);
  gl::GL11::glScalef(10.0f, 10.0f, 10.0f);
  gl::GL11::glTranslatef(1.0f, 0.5f, 1.0f);
  gl::GL11::glScalef(1.0f, 1.0f, -1.0f);
  gl::GL11::glRotatef(210.0f, 1.0f, 0.0f, 0.0f);
  gl::GL11::glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
  applyDisplayColor(stack);
  gl::GL11::glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
  blockRenderManager.ctx.inventoryColorEnabled = useCustomDisplayColor;
  blockRenderManager.ctx.textureManager = &textureManager;
  blockRenderManager.render(*block, stack.getDamage(), 1.0f);
  blockRenderManager.ctx.textureManager = nullptr;
  blockRenderManager.ctx.inventoryColorEnabled = true;
  textureManager.bindTexture(textureManager.getTextureId("/terrain.png"));
  gl::GL11::glPopMatrix();
}
void ItemRenderer::renderSpriteItemInGui(client::texture::TextureManager& textureManager, const ItemStack& stack, int x,
                                         int y) {
  const int sprite = stack.getTextureId();
  if(sprite < 0) {
    return;
  }
  const render::platform::LightingOffGuard lighting;
  textureManager.bindTexture(textureManager.getTextureId(ItemModelRenderer::spriteAtlasPath(stack)));
  applyDisplayColor(stack);
  drawTexture(x, y, (sprite % 16) * 16, (sprite / 16) * 16, 16, 16);
}
void ItemRenderer::applyDisplayColor(const ItemStack& stack) {
  const ItemTint tint = useCustomDisplayColor ? ItemModelRenderer::tintColor(stack) : ItemTint{};
  gl::GL11::glColor4f(tint.red, tint.green, tint.blue, 1.0f);
}
void ItemRenderer::renderGuiItemDecoration(client::font::TextRenderer& textRenderer,
                                           client::texture::TextureManager& textureManager, const ItemStack& stack,
                                           int x, int y) {
  (void)textureManager;
  drawCountLabel(textRenderer, stack, x, y);
  if(stack.isDamaged()) {
    drawDurabilityBar(stack, x, y);
  }
}
void ItemRenderer::drawCountLabel(client::font::TextRenderer& textRenderer, const ItemStack& stack, int x, int y) {
  if(stack.count <= 1) {
    return;
  }
  const std::string label = std::to_string(stack.count);
  gl::GL11::glDisable(gl::GL11::GL_FOG);
  gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
  gl::GL11::glDisable(gl::GL11::GL_DEPTH_TEST);
  textRenderer.drawWithShadow(label, x + 19 - 2 - textRenderer.getWidth(label), y + 6 + 3, 0xFFFFFF);
  gl::GL11::glEnable(gl::GL11::GL_DEPTH_TEST);
  gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
void ItemRenderer::drawDurabilityBar(const ItemStack& stack, int x, int y) {
  const int barPixels = static_cast<int>(
      std::lround(13.0 - static_cast<double>(stack.getDamage2()) * 13.0 / static_cast<double>(stack.getMaxDamage())));
  const int barColorAmount = static_cast<int>(std::lround(255.0 - static_cast<double>(stack.getDamage2()) * 255.0 /
                                                                      static_cast<double>(stack.getMaxDamage())));
  gl::GL11::glDisable(gl::GL11::GL_FOG);
  gl::GL11::glDisable(gl::GL11::GL_DEPTH_TEST);
  gl::GL11::glDisable(gl::GL11::GL_TEXTURE_2D);
  fillRect(x + 2, y + 13, 13, 2, 0);
  fillRect(x + 2, y + 13, 12, 1, ((255 - barColorAmount) / 4 << 16) | 0x3F00);
  fillRect(x + 2, y + 13, barPixels, 1, ((255 - barColorAmount) << 16) | (barColorAmount << 8));
  gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
  gl::GL11::glEnable(gl::GL11::GL_DEPTH_TEST);
  gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
void ItemRenderer::fillRect(int x, int y, int width, int height, int color) {
  Tessellator& tessellator = Tessellator::INSTANCE;
  tessellator.startQuads();
  tessellator.color(color);
  tessellator.vertex(x + 0, y + 0, 0.0);
  tessellator.vertex(x + 0, y + height, 0.0);
  tessellator.vertex(x + width, y + height, 0.0);
  tessellator.vertex(x + width, y + 0, 0.0);
  tessellator.draw();
}
void ItemRenderer::drawTexture(int x, int y, int u, int v, int width, int height) {
  constexpr float depth = 0.0f;
  constexpr float texel = 0.00390625f;
  Tessellator& tessellator = Tessellator::INSTANCE;
  tessellator.startQuads();
  tessellator.vertex(x + 0, y + height, depth, static_cast<float>(u + 0) * texel,
                     static_cast<float>(v + height) * texel);
  tessellator.vertex(x + width, y + height, depth, static_cast<float>(u + width) * texel,
                     static_cast<float>(v + height) * texel);
  tessellator.vertex(x + width, y + 0, depth, static_cast<float>(u + width) * texel,
                     static_cast<float>(v + 0) * texel);
  tessellator.vertex(x + 0, y + 0, depth, static_cast<float>(u + 0) * texel, static_cast<float>(v + 0) * texel);
  tessellator.draw();
}
} // namespace net::minecraft::client::render::item
