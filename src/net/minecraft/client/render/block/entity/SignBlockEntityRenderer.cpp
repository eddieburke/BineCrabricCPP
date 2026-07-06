#include "net/minecraft/client/render/block/entity/SignBlockEntityRenderer.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/entity/SignBlockEntity.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
namespace net::minecraft::client::render::block::entity {
void SignBlockEntityRenderer::render(const net::minecraft::block::entity::BlockEntity& blockEntity, double x, double y,
                                     double z, float tickDelta) {
  (void)tickDelta;
  const auto* sign = dynamic_cast<const net::minecraft::block::entity::SignBlockEntity*>(&blockEntity);
  if(sign == nullptr) {
    return;
  }
  net::minecraft::block::Block* block = sign->getBlock();
  gl::GL11::glPushMatrix();
  const float scale = 0.6666667f;
  if(block == net::minecraft::block::Block::SIGN) {
    gl::GL11::glTranslatef(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.75f * scale,
                           static_cast<float>(z) + 0.5f);
    const float yaw = static_cast<float>(sign->getPushedBlockData() * 360) / 16.0f;
    gl::GL11::glRotatef(-yaw, 0.0f, 1.0f, 0.0f);
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
    gl::GL11::glTranslatef(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.75f * scale,
                           static_cast<float>(z) + 0.5f);
    gl::GL11::glRotatef(-yaw, 0.0f, 1.0f, 0.0f);
    gl::GL11::glTranslatef(0.0f, -0.3125f, -0.4375f);
    model.stick.visible = false;
  }
  bindTexture("/item/sign.png");
  gl::GL11::glPushMatrix();
  gl::GL11::glScalef(scale, -scale, -scale);
  model.render();
  gl::GL11::glPopMatrix();
  font::TextRenderer* textRenderer = getTextRenderer();
  if(textRenderer == nullptr) {
    gl::GL11::glPopMatrix();
    return;
  }
  const float textScale = 0.016666668f * scale;
  gl::GL11::glTranslatef(0.0f, 0.5f * scale, 0.07f * scale);
  gl::GL11::glScalef(textScale, -textScale, textScale);
  gl::GL11::glNormal3f(0.0f, 0.0f, -1.0f * textScale);
  const gl::AttribGuard textState(gl::GL11::GL_ENABLE_BIT | gl::GL11::GL_CURRENT_BIT | gl::GL11::GL_TEXTURE_BIT |
                                  gl::GL11::GL_DEPTH_BUFFER_BIT);
  gl::GL11::glDepthMask(false);
  gl::GL11::glEnable(gl::GL11::GL_BLEND);
  gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
  const int color = 0;
  for(int i = 0; i < 4; ++i) {
    std::string line = sign->texts[static_cast<std::size_t>(i)];
    if(i == sign->currentRow) {
      line = "> " + line + " <";
    }
    textRenderer->draw(line, -textRenderer->getWidth(line) / 2, i * 10 - static_cast<int>(sign->texts.size()) * 5,
                       color);
  }
  gl::GL11::glPopMatrix();
}
} // namespace net::minecraft::client::render::block::entity
