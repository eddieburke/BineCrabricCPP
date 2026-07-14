#include "net/minecraft/client/render/item/ItemRenderer.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/item/ItemModelRenderer.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/mod/ModTexture.hpp"
#include "net/minecraft/mod/model/ModelDraw.hpp"
namespace net::minecraft::client::render::item {
void ItemRenderer::renderGuiItem(client::font::TextRenderer& textRenderer,
                                 client::texture::TextureManager& textureManager,
                                 const ItemStack& stack,
                                 int x,
                                 int y) {
  (void)textRenderer;
  if(stack.count <= 0 || stack.itemId <= 0) {
    return;
  }
  if(ItemModelRenderer::hasCustomModel(stack)) {
    renderCustomModelInGui(textureManager, stack, x, y);
  } else if(ItemModelRenderer::rendersAsBlockModel(stack)) {
    renderBlockItemInGui(textureManager, stack, x, y);
  } else {
    renderSpriteItemInGui(textureManager, stack, x, y);
  }
  gl::setCap(gl::cap::CullFace, true);
}
void ItemRenderer::renderCustomModelInGui(client::texture::TextureManager& textureManager,
                                          const ItemStack& stack,
                                          int x,
                                          int y) {
  if(ItemModelRenderer::usesModTexture(stack)) {
    net::minecraft::mod::bind(textureManager, stack.getTextureId());
  } else {
    textureManager.bindTexture(textureManager.getTextureId(ItemModelRenderer::spriteAtlasPath(stack)));
  }
  const gl::preset::GuiItemRescale rescaleCaps;
  gl::MatrixGuard itemMatrix;
  gl::translatef(static_cast<float>(x - 2), static_cast<float>(y + 3), -3.0f);
  gl::scalef(10.0f, 10.0f, 10.0f);
  gl::translatef(0.5f, 0.5f, 0.5f);
  gl::scalef(1.0f, 1.0f, -1.0f);
  gl::rotatef(210.0f, 1.0f, 0.0f, 0.0f);
  gl::rotatef(45.0f, 0.0f, 1.0f, 0.0f);
  applyDisplayColor(stack);
  gl::rotatef(-90.0f, 0.0f, 1.0f, 0.0f);
  gl::translatef(-0.5f, -0.5f, -0.5f);
  net::minecraft::mod::model::drawLuaItemModel(Tessellator::INSTANCE, stack, 1.0f);
}
void ItemRenderer::renderBlockItemInGui(client::texture::TextureManager& textureManager,
                                        const ItemStack& stack,
                                        int x,
                                        int y) {
  Block* block = ItemModelRenderer::blockOf(stack);
  if(block == nullptr) {
    return;
  }
  textureManager.bindTexture(textureManager.getTextureId("/terrain.png"));
  const bool previousUseAo = blockRenderManager.ctx.faceState.useAo;
  const bool previousInventoryColorEnabled = blockRenderManager.ctx.inventoryColorEnabled;
  auto* previousTextureManager = blockRenderManager.ctx.textureManager;
  const gl::preset::GuiItemRescale rescaleCaps;
  gl::MatrixGuard itemMatrix;
  gl::translatef(static_cast<float>(x - 2), static_cast<float>(y + 3), -3.0f);
  gl::scalef(10.0f, 10.0f, 10.0f);
  gl::translatef(1.0f, 0.5f, 1.0f);
  gl::scalef(1.0f, 1.0f, -1.0f);
  gl::rotatef(210.0f, 1.0f, 0.0f, 0.0f);
  gl::rotatef(45.0f, 0.0f, 1.0f, 0.0f);
  applyDisplayColor(stack);
  gl::rotatef(-90.0f, 0.0f, 1.0f, 0.0f);
  blockRenderManager.ctx.inventoryColorEnabled = useCustomDisplayColor;
  blockRenderManager.ctx.textureManager = &textureManager;
  blockRenderManager.ctx.faceState.useAo = false;
  blockRenderManager.render(*block, stack.getDamage(), 1.0f);
  blockRenderManager.ctx.textureManager = previousTextureManager;
  blockRenderManager.ctx.inventoryColorEnabled = previousInventoryColorEnabled;
  blockRenderManager.ctx.faceState.useAo = previousUseAo;
  textureManager.bindTexture(textureManager.getTextureId("/terrain.png"));
}
void ItemRenderer::renderSpriteItemInGui(client::texture::TextureManager& textureManager,
                                         const ItemStack& stack,
                                         int x,
                                         int y) {
  const int sprite = stack.getTextureId();
  if(sprite < 0) {
    return;
  }
  const gl::preset::ModBlockInventory spriteCaps(true);
  if(ItemModelRenderer::usesModTexture(stack)) {
    net::minecraft::mod::bind(textureManager, sprite);
  } else {
    textureManager.bindTexture(textureManager.getTextureId(ItemModelRenderer::spriteAtlasPath(stack)));
  }
  applyDisplayColor(stack);
  if(ItemModelRenderer::usesModTexture(stack)) {
    Tessellator& tessellator = Tessellator::INSTANCE;
    tessellator.startQuads();
    tessellator.vertex(x + 0, y + 16, 0.0, 0.0, 1.0);
    tessellator.vertex(x + 16, y + 16, 0.0, 1.0, 1.0);
    tessellator.vertex(x + 16, y + 0, 0.0, 1.0, 0.0);
    tessellator.vertex(x + 0, y + 0, 0.0, 0.0, 0.0);
    tessellator.draw();
  } else {
    drawTexture(x, y, (sprite % 16) * 16, (sprite / 16) * 16, 16, 16);
  }
}
void ItemRenderer::applyDisplayColor(const ItemStack& stack) {
  const ItemTint tint = useCustomDisplayColor ? ItemModelRenderer::tintColor(stack) : ItemTint{};
  gl::color4f(tint.red, tint.green, tint.blue, 1.0f);
}
void ItemRenderer::renderGuiItemDecoration(client::font::TextRenderer& textRenderer,
                                           client::texture::TextureManager& textureManager,
                                           const ItemStack& stack,
                                           int x,
                                           int y) {
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
  const gl::preset::GuiItemLabel labelCaps;
  textRenderer.drawWithShadow(label, x + 19 - 2 - textRenderer.getWidth(label), y + 6 + 3, 0xFFFFFF);
}
void ItemRenderer::drawDurabilityBar(const ItemStack& stack, int x, int y) {
  const int barPixels = static_cast<int>(
      std::lround(13.0 - static_cast<double>(stack.getDamage2()) * 13.0 / static_cast<double>(stack.getMaxDamage())));
  const int barColorAmount = static_cast<int>(std::lround(255.0 - static_cast<double>(stack.getDamage2()) * 255.0 /
                                                                      static_cast<double>(stack.getMaxDamage())));
  const gl::preset::GuiDurabilityBar barCaps;
  fillRect(x + 2, y + 13, 13, 2, 0);
  fillRect(x + 2, y + 13, 12, 1, ((255 - barColorAmount) / 4 << 16) | 0x3F00);
  fillRect(x + 2, y + 13, barPixels, 1, ((255 - barColorAmount) << 16) | (barColorAmount << 8));
  gl::color4f(1.0f, 1.0f, 1.0f, 1.0f);
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
  tessellator.vertex(
      x + 0, y + height, depth, static_cast<float>(u + 0) * texel, static_cast<float>(v + height) * texel);
  tessellator.vertex(
      x + width, y + height, depth, static_cast<float>(u + width) * texel, static_cast<float>(v + height) * texel);
  tessellator.vertex(
      x + width, y + 0, depth, static_cast<float>(u + width) * texel, static_cast<float>(v + 0) * texel);
  tessellator.vertex(x + 0, y + 0, depth, static_cast<float>(u + 0) * texel, static_cast<float>(v + 0) * texel);
  tessellator.draw();
}
} // namespace net::minecraft::client::render::item
