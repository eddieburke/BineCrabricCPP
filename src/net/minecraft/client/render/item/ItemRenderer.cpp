#include "net/minecraft/client/render/item/ItemRenderer.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/TextureResolve.hpp"
#include "net/minecraft/client/render/item/ItemModelRenderer.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/mod/model/ModModels.hpp"
#include "net/minecraft/util/math/MatrixStack.hpp"
namespace net::minecraft::client::render::item {
namespace {
void enableGuiItemLighting() {
 core::WorldLightUniforms light = core::worldLight();
 light.sunDirView[0] = 0.16169042f;
 light.sunDirView[1] = 0.80845209f;
 light.sunDirView[2] = -0.56591646f;
 light.fillDirView[0] = -0.16169042f;
 light.fillDirView[1] = 0.80845209f;
 light.fillDirView[2] = 0.56591646f;
 light.sunColor[0] = light.sunColor[1] = light.sunColor[2] = 1.0f;
 light.sunIntensity = 0.6f;
 light.fillIntensity = 0.6f;
 light.ambient[0] = light.ambient[1] = light.ambient[2] = 0.4f;
 core::setWorldLight(light);
 core::setLightingEnabled(true);
}
} // namespace
void ItemRenderer::renderGuiItem(client::font::TextRenderer& textRenderer,
                                 client::texture::TextureManager& textureManager,
                                 const ItemStack& stack,
                                 int x,
                                 int y) {
 (void)textRenderer;
 if(stack.count <= 0 || stack.itemId <= 0) {
  return;
 }
 const render::core::RenderedItemScope itemScope(ItemModelRenderer::shaderId(stack));
 if(ItemModelRenderer::hasCustomModel(stack)) {
  renderCustomModelInGui(textureManager, stack, x, y);
 } else if(ItemModelRenderer::rendersAsBlockModel(stack)) {
  renderBlockItemInGui(textureManager, stack, x, y);
 } else {
  renderSpriteItemInGui(textureManager, stack, x, y);
 }
 core::cullBackFaces();
}
void ItemRenderer::renderCustomModelInGui(client::texture::TextureManager& textureManager,
                                          const ItemStack& stack,
                                          int x,
                                          int y) {
 textureManager.bindTexture(
     resolveBlockTexture(stack.getTextureId(), textureManager, ItemModelRenderer::atlasDomain(stack)).glId);
 enableGuiItemLighting();
 render::RenderPassScope scope(render::RenderType::guiItem3D());
 const core::ScopedDrawCameraState itemGuard;
 net::minecraft::util::math::MatrixStack pose;
 pose.load(core::drawPose());
 pose.translate(static_cast<float>(x - 2), static_cast<float>(y + 3), -3.0f);
 pose.scale(10.0f, 10.0f, 10.0f);
 pose.translate(0.5f, 0.5f, 0.5f);
 pose.scale(1.0f, 1.0f, -1.0f);
 pose.rotate(210.0f, 1.0f, 0.0f, 0.0f);
 pose.rotate(45.0f, 0.0f, 1.0f, 0.0f);
 applyDisplayColor(stack);
 pose.rotate(-90.0f, 0.0f, 1.0f, 0.0f);
 pose.translate(-0.5f, -0.5f, -0.5f);
 core::setDrawPose(pose.top());
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
 enableGuiItemLighting();
 render::RenderPassScope scope(render::RenderType::guiItem3D());
 const core::ScopedDrawCameraState itemGuard;
 net::minecraft::util::math::MatrixStack pose;
 pose.load(core::drawPose());
 pose.translate(static_cast<float>(x - 2), static_cast<float>(y + 3), -3.0f);
 pose.scale(10.0f, 10.0f, 10.0f);
 pose.translate(1.0f, 0.5f, 1.0f);
 pose.scale(1.0f, 1.0f, -1.0f);
 pose.rotate(210.0f, 1.0f, 0.0f, 0.0f);
 pose.rotate(45.0f, 0.0f, 1.0f, 0.0f);
 applyDisplayColor(stack);
 pose.rotate(-90.0f, 0.0f, 1.0f, 0.0f);
 core::setDrawPose(pose.top());
 blockRenderManager.ctx.inventoryColorEnabled = useCustomDisplayColor;
 blockRenderManager.ctx.textureManager = &textureManager;
 blockRenderManager.ctx.faceState.useAo = false;
 // An icon is lit by enableGuiItemLighting() alone -- no world light reaches the
 // inventory. render() reads the light off the tessellator, so say fullbright
 // rather than inheriting whatever the world pass left behind.
 Tessellator::INSTANCE.light(15.0f, 15.0f);
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
 render::RenderPassScope scope(render::RenderType::guiTextured());
 textureManager.bindTexture(
     resolveBlockTexture(sprite, textureManager, ItemModelRenderer::atlasDomain(stack)).glId);
 applyDisplayColor(stack);
 const auto [uMin, uMax, vMin, vMax] = ItemModelRenderer::spriteUv(stack);
 Tessellator& tessellator = Tessellator::INSTANCE;
 tessellator.startQuads();
 tessellator.vertex(x + 0, y + 16, 0.0, uMin, vMax);
 tessellator.vertex(x + 16, y + 16, 0.0, uMax, vMax);
 tessellator.vertex(x + 16, y + 0, 0.0, uMax, vMin);
 tessellator.vertex(x + 0, y + 0, 0.0, uMin, vMin);
 tessellator.draw();
}
void ItemRenderer::applyDisplayColor(const ItemStack& stack) {
 const ItemTint tint = useCustomDisplayColor ? ItemModelRenderer::tintColor(stack) : ItemTint{};
 render::core::setConstColor(tint.red, tint.green, tint.blue, 1.0f);
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
 render::RenderPassScope scope(render::RenderType::guiTextured());
 textRenderer.drawWithShadow(label, x + 19 - 2 - textRenderer.getWidth(label), y + 6 + 3, 0xFFFFFF);
}
void ItemRenderer::drawDurabilityBar(const ItemStack& stack, int x, int y) {
 const int barPixels = static_cast<int>(
     std::lround(13.0 - static_cast<double>(stack.getDamage2()) * 13.0 / static_cast<double>(stack.getMaxDamage())));
 const int barColorAmount = static_cast<int>(std::lround(255.0 - static_cast<double>(stack.getDamage2()) * 255.0 /
                                                                     static_cast<double>(stack.getMaxDamage())));
 render::RenderPassScope scope(render::RenderType::guiTextured());
 fillRect(x + 2, y + 13, 13, 2, 0);
 fillRect(x + 2, y + 13, 12, 1, ((255 - barColorAmount) / 4 << 16) | 0x3F00);
 fillRect(x + 2, y + 13, barPixels, 1, ((255 - barColorAmount) << 16) | (barColorAmount << 8));
 render::core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
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
} // namespace net::minecraft::client::render::item
