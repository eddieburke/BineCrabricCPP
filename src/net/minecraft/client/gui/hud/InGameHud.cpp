#include "net/minecraft/client/gui/hud/InGameHud.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/gui/Draw2D.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/item/ItemRenderer.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/client/util/UiScale.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include <cmath>
#include <array>
#include <vector>
namespace net::minecraft::client::gui::hud {
namespace {
constexpr int kColorWhite = 0xFFFFFF;
constexpr int kColorLightGray = 0xE0E0E0;
int hsbToRgb(float hue, float saturation, float brightness) {
  hue = std::fmod(hue, 1.0f);
  if(hue < 0.0f) {
    hue += 1.0f;
  }
  const int sector = static_cast<int>(hue * 6.0f);
  const float fraction = hue * 6.0f - static_cast<float>(sector);
  const float p = brightness * (1.0f - saturation);
  const float q = brightness * (1.0f - saturation * fraction);
  const float t = brightness * (1.0f - saturation * (1.0f - fraction));
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  switch(sector % 6) {
  case 0:
    r = brightness;
    g = t;
    b = p;
    break;
  case 1:
    r = q;
    g = brightness;
    b = p;
    break;
  case 2:
    r = p;
    g = brightness;
    b = t;
    break;
  case 3:
    r = p;
    g = q;
    b = brightness;
    break;
  case 4:
    r = t;
    g = p;
    b = brightness;
    break;
  default:
    r = brightness;
    g = p;
    b = q;
    break;
  }
  const int ri = static_cast<int>(r * 255.0f) & 0xFF;
  const int gi = static_cast<int>(g * 255.0f) & 0xFF;
  const int bi = static_cast<int>(b * 255.0f) & 0xFF;
  return (ri << 16) | (gi << 8) | bi;
}
void drawFullscreenTexturedQuad(render::Tessellator& tessellator, int width, int height, float z) {
  draw::texturedQuad(tessellator, 0, 0, width, height, 0.0f, 0.0f, 1.0f, 1.0f, z);
}
} // namespace
void InGameHud::tick() {
  if(overlayRemaining > 0) {
    --overlayRemaining;
  }
  ++ticks;
  for(ChatHudLine& line : messages) {
    ++line.age;
  }
}
void InGameHud::clearChat() {
  messages.clear();
}
void InGameHud::addChatMessage(const std::string& message) {
  if(minecraft != nullptr && minecraft->textRenderer != nullptr) {
    std::string remaining = message;
    while(minecraft->textRenderer->getWidth(remaining) > 320) {
      int splitAt = 1;
      for(; splitAt < static_cast<int>(remaining.size()); ++splitAt) {
        if(minecraft->textRenderer->getWidth(remaining.substr(0, splitAt + 1)) > 320) {
          break;
        }
      }
      addChatMessage(remaining.substr(0, splitAt));
      remaining = remaining.substr(static_cast<std::size_t>(splitAt));
    }
    messages.insert(messages.begin(), ChatHudLine(remaining));
  } else {
    messages.insert(messages.begin(), ChatHudLine(message));
  }
  if(messages.size() > 50) {
    messages.resize(50);
  }
}
void InGameHud::addTranslatedChatMessage(const std::string& text) {
  addChatMessage(resource::language::I18n::getTranslation(text));
}
void InGameHud::setRecordPlayingOverlay(const std::string& record) {
  overlayMessage = "Now playing: " + record;
  overlayRemaining = 60;
  overlayTinted = true;
}
void InGameHud::renderHotbarItem(int slot, int x, int y, float tickDelta) {
  if(minecraft == nullptr || minecraft->player == nullptr || minecraft->textRenderer == nullptr) {
    return;
  }
  ItemStack& stack = minecraft->player->inventory.main[static_cast<std::size_t>(slot)];
  if(stack.empty()) {
    return;
  }
  static render::item::ItemRenderer itemRenderer;
  const float bobTime = static_cast<float>(stack.bobbingAnimationTime) - tickDelta;
  if(bobTime > 0.0f) {
    gl::GL11::glPushMatrix();
    const float scale = 1.0f + bobTime / 5.0f;
    gl::GL11::glTranslatef(static_cast<float>(x + 8), static_cast<float>(y + 12), 0.0f);
    gl::GL11::glScalef(1.0f / scale, (scale + 1.0f) / 2.0f, 1.0f);
    gl::GL11::glTranslatef(static_cast<float>(-(x + 8)), static_cast<float>(-(y + 12)), 0.0f);
  }
  itemRenderer.renderGuiItem(*minecraft->textRenderer, minecraft->textureManager, stack, x, y);
  if(bobTime > 0.0f) {
    gl::GL11::glPopMatrix();
  }
  itemRenderer.renderGuiItemDecoration(*minecraft->textRenderer, minecraft->textureManager, stack, x, y);
}
void InGameHud::renderVignette(float brightness, int width, int height) {
  float darkness = 1.0f - brightness;
  darkness = std::clamp(darkness, 0.0f, 1.0f);
  vignetteDarkness = static_cast<float>(static_cast<double>(vignetteDarkness) +
                                        static_cast<double>(darkness - vignetteDarkness) * 0.01);
  gl::GL11::glDisable(gl::GL11::GL_DEPTH_TEST);
  gl::GL11::glDepthMask(false);
  gl::GL11::glEnable(gl::GL11::GL_BLEND);
  gl::GL11::glDisable(gl::GL11::GL_ALPHA_TEST);
  gl::GL11::glBlendFunc(gl::GL11::GL_ZERO, gl::GL11::GL_ONE_MINUS_SRC_COLOR);
  gl::GL11::glColor4f(vignetteDarkness, vignetteDarkness, vignetteDarkness, 1.0f);
  gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D,
                          minecraft->textureManager.getTextureId("%blur%/misc/vignette.png"));
  drawFullscreenTexturedQuad(render::Tessellator::INSTANCE, width, height, -90.0f);
  gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
  gl::GL11::glDepthMask(true);
  gl::GL11::glEnable(gl::GL11::GL_DEPTH_TEST);
  gl::GL11::glEnable(gl::GL11::GL_ALPHA_TEST);
  gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
void InGameHud::renderPumpkinOverlay(int width, int height) {
  gl::GL11::glDisable(gl::GL11::GL_DEPTH_TEST);
  gl::GL11::glDepthMask(false);
  gl::GL11::glEnable(gl::GL11::GL_BLEND);
  gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
  gl::GL11::glDisable(gl::GL11::GL_ALPHA_TEST);
  gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D,
                          minecraft->textureManager.getTextureId("%blur%/misc/pumpkinblur.png"));
  drawFullscreenTexturedQuad(render::Tessellator::INSTANCE, width, height, -90.0f);
  gl::GL11::glDepthMask(true);
  gl::GL11::glEnable(gl::GL11::GL_DEPTH_TEST);
  gl::GL11::glEnable(gl::GL11::GL_ALPHA_TEST);
  gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
void InGameHud::renderPortalOverlay(float strength, int width, int height) {
  float alpha = strength;
  if(alpha < 1.0f) {
    alpha *= alpha;
    alpha *= alpha;
    alpha = alpha * 0.8f + 0.2f;
  }
  if(Block::NETHER_PORTAL == nullptr) {
    return;
  }
  gl::GL11::glDisable(gl::GL11::GL_DEPTH_TEST);
  gl::GL11::glDepthMask(false);
  gl::GL11::glEnable(gl::GL11::GL_BLEND);
  gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
  gl::GL11::glDisable(gl::GL11::GL_ALPHA_TEST);
  gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, alpha);
  gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, minecraft->textureManager.getTextureId("/terrain.png"));
  const int textureIndex = Block::NETHER_PORTAL->textureId;
  const float uMin = static_cast<float>(textureIndex % 16) / 16.0f;
  const float vMin = static_cast<float>(textureIndex / 16) / 16.0f;
  const float uMax = static_cast<float>(textureIndex % 16 + 1) / 16.0f;
  const float vMax = static_cast<float>(textureIndex / 16 + 1) / 16.0f;
  render::Tessellator& tessellator = render::Tessellator::INSTANCE;
  draw::texturedQuad(tessellator, 0, 0, width, height, uMin, vMin, uMax, vMax, -90.0f);
  gl::GL11::glDepthMask(true);
  gl::GL11::glEnable(gl::GL11::GL_DEPTH_TEST);
  gl::GL11::glEnable(gl::GL11::GL_ALPHA_TEST);
  gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
void InGameHud::renderDebugHud(font::TextRenderer& textRenderer, int /*scaledWidth*/,
                               const entity::player::PlayerEntity& player) {
  gl::GL11::glDisable(gl::GL11::GL_FOG);
  gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
  gl::GL11::glPushMatrix();
  if(Minecraft::failedSessionCheckTime().load(std::memory_order_relaxed) > 0) {
    gl::GL11::glTranslatef(0.0f, 32.0f, 0.0f);
  }
  drawTextWithShadow(textRenderer, "Minecraft Beta 1.7.3 (" + minecraft->debugText + ")", 2, 2, kColorWhite);
  drawTextWithShadow(textRenderer, minecraft->getRenderChunkDebugInfo(), 2, 12, kColorWhite);
  drawTextWithShadow(textRenderer, minecraft->getRenderEntityDebugInfo(), 2, 22, kColorWhite);
  drawTextWithShadow(textRenderer, minecraft->getWorldDebugInfo(), 2, 32, kColorWhite);
  drawTextWithShadow(textRenderer, minecraft->getChunkSourceDebugInfo(), 2, 42, kColorWhite);
  drawTextWithShadow(textRenderer, "x: " + std::to_string(player.x), 2, 64, kColorLightGray);
  drawTextWithShadow(textRenderer, "y: " + std::to_string(player.y), 2, 72, kColorLightGray);
  drawTextWithShadow(textRenderer, "z: " + std::to_string(player.z), 2, 80, kColorLightGray);
  const int facing = (MathHelper::floor(static_cast<double>(player.yaw * 4.0f / 360.0f) + 0.5) & 3);
  drawTextWithShadow(textRenderer, "f: " + std::to_string(facing), 2, 88, kColorLightGray);
  gl::GL11::glPopMatrix();
  gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
void InGameHud::renderRecordOverlay(font::TextRenderer& textRenderer, float tickDelta, int scaledWidth,
                                    int scaledHeight) {
  float remaining = static_cast<float>(overlayRemaining) - tickDelta;
  int alpha = static_cast<int>(remaining * 256.0f / 20.0f);
  if(alpha > 255) {
    alpha = 255;
  }
  if(alpha <= 0) {
    return;
  }
  gl::GL11::glPushMatrix();
  gl::GL11::glTranslatef(static_cast<float>(scaledWidth) / 2.0f, static_cast<float>(scaledHeight - 48), 0.0f);
  gl::GL11::glEnable(gl::GL11::GL_BLEND);
  gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
  gl::GL11::glDisable(gl::GL11::GL_ALPHA_TEST);
  int color = kColorWhite;
  if(overlayTinted) {
    color = hsbToRgb(remaining / 50.0f, 0.7f, 0.6f);
  }
  textRenderer.draw(overlayMessage, -textRenderer.getWidth(overlayMessage) / 2, -4, color + (alpha << 24));
  gl::GL11::glEnable(gl::GL11::GL_ALPHA_TEST);
  gl::GL11::glDisable(gl::GL11::GL_BLEND);
  gl::GL11::glPopMatrix();
}
void InGameHud::renderChat(font::TextRenderer& textRenderer, bool chatOpen, int scaledWidth, int scaledHeight) {
  (void)scaledWidth;
  int maxLines = 10;
  if(chatOpen) {
    maxLines = 20;
  }
  gl::GL11::glEnable(gl::GL11::GL_BLEND);
  gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
  gl::GL11::glDisable(gl::GL11::GL_ALPHA_TEST);
  gl::GL11::glPushMatrix();
  gl::GL11::glTranslatef(0.0f, static_cast<float>(scaledHeight - 48), 0.0f);
  struct VisibleLine {
    int y = 0;
    int alpha = 0;
    std::size_t index = 0;
  };
  std::vector<VisibleLine> visible;
  visible.reserve(static_cast<std::size_t>(maxLines));
  for(int i = 0; i < static_cast<int>(messages.size()) && i < maxLines; ++i) {
    if(messages[static_cast<std::size_t>(i)].age >= 200 && !chatOpen) {
      continue;
    }
    double fade = static_cast<double>(messages[static_cast<std::size_t>(i)].age) / 200.0;
    fade = 1.0 - fade;
    fade *= 10.0;
    fade = std::clamp(fade, 0.0, 1.0);
    fade *= fade;
    int alpha = static_cast<int>(255.0 * fade);
    if(chatOpen) {
      alpha = 255;
    }
    if(alpha <= 0) {
      continue;
    }
    visible.push_back({-i * 9, alpha, static_cast<std::size_t>(i)});
  }
  if(!visible.empty()) {
    render::Tessellator& tessellator = render::Tessellator::INSTANCE;
    gl::GL11::glDisable(gl::GL11::GL_TEXTURE_2D);
    tessellator.startQuads();
    for(const VisibleLine& line : visible) {
      draw::appendColoredQuad(tessellator, 2, line.y - 1, 322, line.y + 8, 0, line.alpha / 2, zOffset);
    }
    tessellator.draw();
    gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
  }
  for(const VisibleLine& line : visible) {
    drawTextWithShadow(textRenderer, messages[line.index].text, 2, line.y, kColorWhite + (line.alpha << 24));
  }
  gl::GL11::glPopMatrix();
  gl::GL11::glEnable(gl::GL11::GL_ALPHA_TEST);
  gl::GL11::glDisable(gl::GL11::GL_BLEND);
}
void InGameHud::render(float tickDelta, bool screenOpen, int mouseX, int mouseY) {
  (void)mouseX;
  (void)mouseY;
  if(minecraft == nullptr || minecraft->player == nullptr || minecraft->textRenderer == nullptr) {
    return;
  }
  if(minecraft->options.hideHud) {
    return;
  }
  const util::UiScale scale =
      util::uiScale(minecraft->options, minecraft->displayWidth, minecraft->displayHeight);
  const int scaledWidth = scale.scaledWidth;
  const int scaledHeight = scale.scaledHeight;
  font::TextRenderer& textRenderer = *minecraft->textRenderer;
  PlayerEntity& player = *minecraft->player;
  if(minecraft->gameRenderer != nullptr) {
    minecraft->gameRenderer->setupHudRender();
  }
  gl::GL11::glEnable(gl::GL11::GL_BLEND);
  gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
  if(Minecraft::isFancyGraphicsEnabled()) {
    renderVignette(player.getBrightnessAtEyes(tickDelta), scaledWidth, scaledHeight);
  }
  const ItemStack& helmet = player.inventory.armor[3];
  if(!minecraft->options.thirdPerson && !helmet.empty() && Block::PUMPKIN != nullptr &&
     helmet.itemId == Block::PUMPKIN->id) {
    renderPumpkinOverlay(scaledWidth, scaledHeight);
  }
  const float portalStrength =
      player.lastScreenDistortion + (player.screenDistortion - player.lastScreenDistortion) * tickDelta;
  if(portalStrength > 0.0f) {
    renderPortalOverlay(portalStrength, scaledWidth, scaledHeight);
  }
  gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, minecraft->textureManager.getTextureId("/gui/gui.png"));
  zOffset = -90.0f;
  const std::array<draw::AtlasRect, 2> hotbarSprites{
      draw::AtlasRect{scaledWidth / 2 - 91, scaledHeight - 22, 0, 0, 182, 22},
      draw::AtlasRect{scaledWidth / 2 - 91 - 1 + player.inventory.selectedSlot * 20, scaledHeight - 22 - 1, 0, 22, 24,
                      22},
  };
  drawTextures(hotbarSprites);
  gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, minecraft->textureManager.getTextureId("/gui/icons.png"));
  gl::GL11::glEnable(gl::GL11::GL_BLEND);
  gl::GL11::glBlendFunc(gl::GL11::GL_ONE_MINUS_DST_COLOR, gl::GL11::GL_ONE_MINUS_SRC_COLOR);
  drawTexture(scaledWidth / 2 - 7, scaledHeight / 2 - 7, 0, 0, 16, 16);
  gl::GL11::glDisable(gl::GL11::GL_BLEND);
  bool blinkHearts = player.hearts / 3 % 2 == 1;
  if(player.hearts < 10) {
    blinkHearts = false;
  }
  const int health = player.health;
  const int lastHealth = player.lastHealth;
  random.seed(static_cast<std::uint32_t>(ticks * 312871U));
  if(minecraft->interactionManager != nullptr && minecraft->interactionManager->canBeRendered()) {
    std::vector<draw::AtlasRect> statusSprites;
    statusSprites.reserve(64);
    const int armor = player.inventory.getTotalArmorDurability();
    for(int heart = 0; heart < 10; ++heart) {
      int y = scaledHeight - 32;
      if(armor > 0) {
        const int armorX = scaledWidth / 2 + 91 - heart * 8 - 9;
        if(heart * 2 + 1 < armor) {
          statusSprites.push_back({armorX, y, 34, 9, 9, 9});
        } else if(heart * 2 + 1 == armor) {
          statusSprites.push_back({armorX, y, 25, 9, 9, 9});
        } else if(heart * 2 + 1 > armor) {
          statusSprites.push_back({armorX, y, 16, 9, 9, 9});
        }
      }
      const int blink = blinkHearts ? 1 : 0;
      const int heartX = scaledWidth / 2 - 91 + heart * 8;
      if(health <= 4) {
        y += static_cast<int>(random() & 1U);
      }
      statusSprites.push_back({heartX, y, 16 + blink * 9, 0, 9, 9});
      if(blinkHearts) {
        if(heart * 2 + 1 < lastHealth) {
          statusSprites.push_back({heartX, y, 70, 0, 9, 9});
        } else if(heart * 2 + 1 == lastHealth) {
          statusSprites.push_back({heartX, y, 79, 0, 9, 9});
        }
      }
      if(heart * 2 + 1 < health) {
        statusSprites.push_back({heartX, y, 52, 0, 9, 9});
      } else if(heart * 2 + 1 == health) {
        statusSprites.push_back({heartX, y, 61, 0, 9, 9});
      }
    }
    if(player.isInFluid(block::material::Material::WATER)) {
      const int depleted = static_cast<int>(std::ceil(static_cast<double>(player.air - 2) * 10.0 / 300.0));
      const int remaining =
          static_cast<int>(std::ceil(static_cast<double>(player.air) * 10.0 / 300.0)) - depleted;
      for(int bubble = 0; bubble < depleted + remaining; ++bubble) {
        const int bubbleX = scaledWidth / 2 - 91 + bubble * 8;
        const int bubbleY = scaledHeight - 32 - 9;
        if(bubble < depleted) {
          statusSprites.push_back({bubbleX, bubbleY, 16, 18, 9, 9});
        } else {
          statusSprites.push_back({bubbleX, bubbleY, 25, 18, 9, 9});
        }
      }
    }
    drawTextures(statusSprites);
  }
  gl::GL11::glDisable(gl::GL11::GL_BLEND);
  render::platform::Lighting::turnOn();
  for(int slot = 0; slot < 9; ++slot) {
    const int x = scaledWidth / 2 - 90 + slot * 20 + 2;
    const int y = scaledHeight - 16 - 3;
    renderHotbarItem(slot, x, y, tickDelta);
  }
  render::platform::Lighting::turnOff();
  if(player.getSleepTimer() > 0) {
    gl::GL11::glDisable(gl::GL11::GL_DEPTH_TEST);
    gl::GL11::glDisable(gl::GL11::GL_ALPHA_TEST);
    const int sleepTimer = player.getSleepTimer();
    float fade = static_cast<float>(sleepTimer) / 100.0f;
    if(fade > 1.0f) {
      fade = 1.0f - static_cast<float>(sleepTimer - 100) / 10.0f;
    }
    const int color = (static_cast<int>(220.0f * fade) << 24) | 0x101020;
    fill(0, 0, scaledWidth, scaledHeight, static_cast<std::uint32_t>(color));
    gl::GL11::glEnable(gl::GL11::GL_ALPHA_TEST);
    gl::GL11::glEnable(gl::GL11::GL_DEPTH_TEST);
  }
  if(minecraft->options.debugHud) {
    renderDebugHud(textRenderer, scaledWidth, player);
  }
  if(overlayRemaining > 0 && !overlayMessage.empty()) {
    renderRecordOverlay(textRenderer, tickDelta, scaledWidth, scaledHeight);
  }
  renderChat(textRenderer, screenOpen, scaledWidth, scaledHeight);
  gl::GL11::glDisable(gl::GL11::GL_BLEND);
  gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
} // namespace net::minecraft::client::gui::hud
