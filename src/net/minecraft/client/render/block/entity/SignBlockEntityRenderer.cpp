#include "net/minecraft/client/render/block/entity/SignBlockEntityRenderer.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/entity/SignBlockEntity.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
namespace net::minecraft::client::render::block::entity {
void SignBlockEntityRenderer::render(
    const net::minecraft::block::entity::BlockEntity& blockEntity, double x, double y, double z, float tickDelta) {
 (void)tickDelta;
 const auto* sign = dynamic_cast<const net::minecraft::block::entity::SignBlockEntity*>(&blockEntity);
 if(sign == nullptr) {
  return;
 }
 net::minecraft::block::Block* block = sign->getBlock();
 render::core::modelViewStack().push();
 const float scale = 0.6666667f;
 if(block == net::minecraft::block::Block::SIGN) {
  render::core::modelViewStack().translate(
      static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.75f * scale, static_cast<float>(z) + 0.5f);
  const float yaw = static_cast<float>(sign->getPushedBlockData() * 360) / 16.0f;
  render::core::modelViewStack().rotate(-yaw, 0.0f, 1.0f, 0.0f);
  model.stick.visible = true;
 } else {
  const int meta = sign->getPushedBlockData();
  float yaw = 0.0f;
  if(meta == 2) {
   yaw = 180.0f;
  } else if(meta == 4) {
   yaw = 90.0f;
  } else if(meta == 5) {
   yaw = -90.0f;
  }
  render::core::modelViewStack().translate(
      static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.75f * scale, static_cast<float>(z) + 0.5f);
  render::core::modelViewStack().rotate(-yaw, 0.0f, 1.0f, 0.0f);
  render::core::modelViewStack().translate(0.0f, -0.3125f, -0.4375f);
  model.stick.visible = false;
 }
 bindTexture("/item/sign.png");
 render::core::modelViewStack().push();
 render::core::modelViewStack().scale(scale, -scale, -scale);
 model.render();
 render::core::modelViewStack().pop();
 font::TextRenderer* textRenderer = getTextRenderer();
 if(textRenderer == nullptr) {
  render::core::modelViewStack().pop();
  return;
 }
 const float textScale = 0.016666668f * scale;
 render::core::modelViewStack().translate(0.0f, 0.5f * scale, 0.07f * scale);
 render::core::modelViewStack().scale(textScale, -textScale, textScale);
 render::RenderPassScope passScope(render::RenderType::text());
 render::core::enableDepthTest();
 render::core::depthMask(false);
 const int color = 0;
 for(int i = 0; i < 4; ++i) {
  std::string line = sign->texts[static_cast<std::size_t>(i)];
  if(i == sign->currentRow) {
   line = "> " + line + " <";
  }
  textRenderer->draw(
      line, -textRenderer->getWidth(line) / 2, i * 10 - static_cast<int>(sign->texts.size()) * 5, color);
 }
 render::core::modelViewStack().pop();
}
} // namespace net::minecraft::client::render::block::entity
