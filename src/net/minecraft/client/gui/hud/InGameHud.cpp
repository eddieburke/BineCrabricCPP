#include "net/minecraft/client/gui/hud/InGameHud.hpp"
#include <array>
#include <cmath>
#include <string_view>
#include <vector>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gui/Draw2D.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/camera/GuiProjection.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/item/ItemRenderer.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/client/util/UiScale.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/mod/ScreenUi.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
namespace net::minecraft::client::gui::hud {
namespace core = net::minecraft::client::render::core;
namespace {
constexpr int kColorWhite = 0xFFFFFF;
constexpr int kColorLightGray = 0xE0E0E0;
constexpr int kColorLink = 0x55AAFF;
std::pair<std::size_t, std::size_t> chatLinkRange(std::string_view text) {
 const std::size_t secure = text.find("https://");
 const std::size_t plain = text.find("http://");
 std::size_t start = std::string_view::npos;
 if(secure != std::string_view::npos && plain != std::string_view::npos) {
  start = std::min(secure, plain);
 } else if(secure != std::string_view::npos) {
  start = secure;
 } else {
  start = plain;
 }
 if(start == std::string_view::npos) {
  return {start, start};
 }
 std::size_t end = text.find_first_of(" \t\r\n", start);
 if(end == std::string_view::npos) {
  end = text.size();
 }
 while(end > start && std::string_view(".,!?;:)]}").find(text[end - 1]) != std::string_view::npos) {
  --end;
 }
 return end > start ? std::pair{start, end} : std::pair{std::string_view::npos, std::string_view::npos};
}
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
 sentMessages_.clear();
 chatScroll_ = 0;
}
void InGameHud::addChatMessage(const std::string& message) {
 net::minecraft::mod::ChatEvent event;
 event.world = minecraft != nullptr ? minecraft->world : nullptr;
 event.message = message;
 net::minecraft::mod::runtime::luaHookChatReceive(event);
 if(event.canceled) {
  return;
 }
 std::string remaining = std::move(event.message);
 if(minecraft != nullptr && minecraft->textRenderer != nullptr) {
  while(minecraft->textRenderer->getWidth(remaining) > 320) {
   int splitAt = 1;
   for(; splitAt < static_cast<int>(remaining.size()); ++splitAt) {
    if(minecraft->textRenderer->getWidth(remaining.substr(0, splitAt + 1)) > 320) {
     break;
    }
   }
   messages.insert(messages.begin(), ChatHudLine(remaining.substr(0, splitAt)));
   remaining = remaining.substr(static_cast<std::size_t>(splitAt));
  }
 }
 messages.insert(messages.begin(), ChatHudLine(remaining));
 if(messages.size() > 512) {
  messages.resize(512);
 }
 resetChatScroll();
}
void InGameHud::addSentChatMessage(std::string message) {
 if(message.empty()) {
  return;
 }
 if(sentMessages_.empty() || sentMessages_.back() != message) {
  sentMessages_.push_back(std::move(message));
 }
 if(sentMessages_.size() > 100) {
  sentMessages_.erase(sentMessages_.begin(), sentMessages_.begin() + 1);
 }
}
void InGameHud::scrollChat(int lines) {
 const int maxScroll = std::max(0, static_cast<int>(messages.size()) - 20);
 chatScroll_ = std::clamp(chatScroll_ + lines, 0, maxScroll);
}
void InGameHud::resetChatScroll() {
 chatScroll_ = 0;
}
std::optional<std::size_t> InGameHud::chatLineIndexAt(int mouseX, int mouseY, int scaledHeight) const {
 if(mouseX < 2 || mouseX > 322) {
  return std::nullopt;
 }
 const int baseY = scaledHeight - 48;
 for(int displayed = 0; displayed < 20; ++displayed) {
  const int y = baseY - displayed * 9;
  if(mouseY >= y - 1 && mouseY <= y + 8) {
   const std::size_t index = static_cast<std::size_t>(chatScroll_ + displayed);
   if(index < messages.size()) {
    return index;
   }
   return std::nullopt;
  }
 }
 return std::nullopt;
}
std::string InGameHud::chatLineAt(int mouseX, int mouseY, int scaledHeight) const {
 const std::optional<std::size_t> index = chatLineIndexAt(mouseX, mouseY, scaledHeight);
 return index ? messages[*index].text : std::string{};
}
std::string InGameHud::chatLinkAt(int mouseX, int mouseY, int scaledHeight) const {
 if(minecraft == nullptr || minecraft->textRenderer == nullptr) {
  return {};
 }
 const std::optional<std::size_t> index = chatLineIndexAt(mouseX, mouseY, scaledHeight);
 if(!index) {
  return {};
 }
 const std::string& text = messages[*index].text;
 const auto [start, end] = chatLinkRange(text);
 if(start == std::string::npos) {
  return {};
 }
 const int left = 2 + minecraft->textRenderer->getWidth(text.substr(0, start));
 const int right = left + minecraft->textRenderer->getWidth(text.substr(start, end - start));
 return mouseX >= left && mouseX < right ? text.substr(start, end - start) : std::string{};
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
  const core::ScopedDrawCameraState bobGuard;
  net::minecraft::util::math::Matrix4f bobPose = core::drawPose();
  const float scale = 1.0f + bobTime / 5.0f;
  bobPose.translate(static_cast<float>(x + 8), static_cast<float>(y + 12), 0.0f);
  bobPose.scale(1.0f / scale, (scale + 1.0f) / 2.0f, 1.0f);
  bobPose.translate(static_cast<float>(-(x + 8)), static_cast<float>(-(y + 12)), 0.0f);
  core::setDrawPose(bobPose);
 }
 itemRenderer.renderGuiItem(*minecraft->textRenderer, minecraft->textureManager, stack, x, y);
 itemRenderer.renderGuiItemDecoration(*minecraft->textRenderer, minecraft->textureManager, stack, x, y);
}
void InGameHud::renderVignette(float brightness, int width, int height) {
 float darkness = 1.0f - brightness;
 darkness = std::clamp(darkness, 0.0f, 1.0f);
 vignetteDarkness = static_cast<float>(static_cast<double>(vignetteDarkness) +
                                       static_cast<double>(darkness - vignetteDarkness) * 0.01);
 const render::RenderPassScope guiPass(render::RenderType::guiTextured());
 core::depthMask(false);
 core::blendInverseColor();
 core::setConstColor(vignetteDarkness, vignetteDarkness, vignetteDarkness, 1.0f);
 minecraft->textureManager.bindTexture(minecraft->textureManager.getTextureId("%blur%/misc/vignette.png"));
 drawFullscreenTexturedQuad(render::Tessellator::INSTANCE, width, height, -90.0f);
 core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
}
void InGameHud::renderPumpkinOverlay(int width, int height) {
 const render::RenderPassScope guiPass(render::RenderType::guiTextured());
 core::depthMask(false);
 minecraft->textureManager.bindTexture(minecraft->textureManager.getTextureId("%blur%/misc/pumpkinblur.png"));
 drawFullscreenTexturedQuad(render::Tessellator::INSTANCE, width, height, -90.0f);
 core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
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
 const render::RenderPassScope guiPass(render::RenderType::guiTextured());
 core::depthMask(false);
 core::setConstColor(1.0f, 1.0f, 1.0f, alpha);
 minecraft->textureManager.bindTexture(minecraft->textureManager.getTextureId("/terrain.png"));
 const int textureIndex = Block::NETHER_PORTAL->textureId;
 const float uMin = static_cast<float>(textureIndex % 16) / 16.0f;
 const float vMin = static_cast<float>(textureIndex / 16) / 16.0f;
 const float uMax = static_cast<float>(textureIndex % 16 + 1) / 16.0f;
 const float vMax = static_cast<float>(textureIndex / 16 + 1) / 16.0f;
 render::Tessellator& tessellator = render::Tessellator::INSTANCE;
 draw::texturedQuad(tessellator, 0, 0, width, height, uMin, vMin, uMax, vMax, -90.0f);
 core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
}
void InGameHud::renderDebugHud(font::TextRenderer& textRenderer,
                               const entity::player::PlayerEntity& player) {
 {
  const core::ScopedDrawCameraState debugGuard;
  if(Minecraft::failedSessionCheckTime().load(std::memory_order_relaxed) > 0) {
   net::minecraft::util::math::Matrix4f debugPose = core::drawPose();
   debugPose.translate(0.0f, 32.0f, 0.0f);
   core::setDrawPose(debugPose);
  }
  textRenderer.drawWithShadow("Minecraft Beta 1.7.3 (" + minecraft->debugText + ")", 2, 2, kColorWhite);
  textRenderer.drawWithShadow(minecraft->getRenderChunkDebugInfo(), 2, 12, kColorWhite);
  textRenderer.drawWithShadow(minecraft->getRenderEntityDebugInfo(), 2, 22, kColorWhite);
  textRenderer.drawWithShadow(minecraft->getWorldDebugInfo(), 2, 32, kColorWhite);
  textRenderer.drawWithShadow(minecraft->getChunkSourceDebugInfo(), 2, 42, kColorWhite);
   textRenderer.drawWithShadow("X: " + std::to_string(player.x), 2, 64, kColorLightGray);
   textRenderer.drawWithShadow("Y: " + std::to_string(player.y), 2, 72, kColorLightGray);
   textRenderer.drawWithShadow("Z: " + std::to_string(player.z), 2, 80, kColorLightGray);
   const int facing = (MathHelper::floor(static_cast<double>(player.yaw * 4.0f / 360.0f) + 0.5) & 3);
   static const char* kFacingNames[] = {"South", "West", "North", "East"};
   textRenderer.drawWithShadow(
       "Facing: " + std::string(kFacingNames[facing]) + " (" + std::to_string(facing) + ")", 2, 88,
       kColorLightGray);
 }
 core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
}
void InGameHud::renderRecordOverlay(font::TextRenderer& textRenderer,
                                    float tickDelta,
                                    int scaledWidth,
                                    int scaledHeight) {
 float remaining = static_cast<float>(overlayRemaining) - tickDelta;
 int alpha = static_cast<int>(remaining * 256.0f / 20.0f);
 if(alpha > 255) {
  alpha = 255;
 }
 if(alpha <= 0) {
  return;
 }
  {
   const core::ScopedDrawCameraState recordGuard;
   net::minecraft::util::math::Matrix4f recordPose = core::drawPose();
   recordPose.translate(static_cast<float>(scaledWidth) / 2.0f, static_cast<float>(scaledHeight - 48), 0.0f);
   core::setDrawPose(recordPose);
   int color = kColorWhite;
  if(overlayTinted) {
   color = hsbToRgb(remaining / 50.0f, 0.7f, 0.6f);
  }
  textRenderer.draw(overlayMessage, -textRenderer.getWidth(overlayMessage) / 2, -4, color + (alpha << 24));
 }
}
void InGameHud::renderChat(font::TextRenderer& textRenderer, bool chatOpen, int scaledWidth, int scaledHeight) {
 (void)scaledWidth;
 int maxLines = 10;
 if(chatOpen) {
  maxLines = 20;
 }
 const core::BlendScope chatCaps(true);
 {
  const core::ScopedDrawCameraState chatGuard;
  net::minecraft::util::math::Matrix4f chatPose = core::drawPose();
  chatPose.translate(0.0f, static_cast<float>(scaledHeight - 48), 0.0f);
  core::setDrawPose(chatPose);
  struct VisibleLine {
   int y = 0;
   int alpha = 0;
   std::size_t index = 0;
  };
  std::vector<VisibleLine> visible;
  visible.reserve(static_cast<std::size_t>(maxLines));
  const int start = chatOpen ? chatScroll_ : 0;
  for(int i = start, displayed = 0;
      i < static_cast<int>(messages.size()) && displayed < maxLines;
      ++i) {
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
   visible.push_back({-displayed * 9, alpha, static_cast<std::size_t>(i)});
   ++displayed;
  }
  if(!visible.empty()) {
   render::Tessellator& tessellator = render::Tessellator::INSTANCE;
   const render::RenderPassScope chatBgPass(render::RenderType::gui());
   tessellator.startQuads();
   for(const VisibleLine& line : visible) {
    draw::appendColoredQuad(tessellator, 2, line.y - 1, 322, line.y + 8, 0, line.alpha / 2, -90.0f);
   }
   tessellator.draw();
  }
  for(const VisibleLine& line : visible) {
   const std::string& text = messages[line.index].text;
   const auto [linkStart, linkEnd] = chatLinkRange(text);
   if(linkStart == std::string::npos) {
    textRenderer.drawWithShadow(text, 2, line.y, kColorWhite + (line.alpha << 24));
    continue;
   }
   int x = 2;
   if(linkStart > 0) {
    const std::string prefix = text.substr(0, linkStart);
    textRenderer.drawWithShadow(prefix, x, line.y, kColorWhite + (line.alpha << 24));
    x += textRenderer.getWidth(prefix);
   }
   const std::string link = text.substr(linkStart, linkEnd - linkStart);
   textRenderer.drawWithShadow(link, x, line.y, kColorLink + (line.alpha << 24));
   x += textRenderer.getWidth(link);
   if(linkEnd < text.size()) {
    textRenderer.drawWithShadow(text.substr(linkEnd), x, line.y, kColorWhite + (line.alpha << 24));
   }
  }
 }
 core::disableBlend();
}
void InGameHud::render(float tickDelta, bool screenOpen, int mouseX, int mouseY) {
 if(minecraft == nullptr || minecraft->player == nullptr || minecraft->textRenderer == nullptr) {
  return;
 }
 if(minecraft->options.hideHud) {
  return;
 }
 const util::UiScale scale = util::uiScale(minecraft->options, minecraft->displayWidth, minecraft->displayHeight);
 const int scaledWidth = scale.scaledWidth;
 const int scaledHeight = scale.scaledHeight;
 font::TextRenderer& textRenderer = *minecraft->textRenderer;
 PlayerEntity& player = *minecraft->player;
 if(minecraft->gameRenderer != nullptr) {
  {
   const int width = std::max(1, minecraft->displayWidth);
   const int height = std::max(1, minecraft->displayHeight);
   render::gui_proj::begin(util::uiScale(minecraft->options, width, height), width, height, false);
  }
 }
 const core::BlendScope hudPass(true);
 const bool drawVignette = Minecraft::isFancyGraphicsEnabled() &&
                           (minecraft->gameRenderer == nullptr || minecraft->gameRenderer->frameSettings().vignette);
 if(drawVignette) {
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
 constexpr float kHudZ = -90.0f;
 core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
 minecraft->textureManager.bindTexture(minecraft->textureManager.getTextureId("/gui/gui.png"));
 const std::array<draw::AtlasRect, 2> hotbarSprites{
     draw::AtlasRect{scaledWidth / 2 - 91, scaledHeight - 22, 0, 0, 182, 22},
     draw::AtlasRect{
         scaledWidth / 2 - 91 - 1 + player.inventory.selectedSlot * 20, scaledHeight - 22 - 1, 0, 22, 24, 22},
 };
 {
  const render::RenderPassScope passScope(render::RenderType::guiTextured());
  const float* c = core::constColor();
  render::Tessellator& tess = render::Tessellator::INSTANCE;
  tess.startQuads();
  tess.color(c[0], c[1], c[2], c[3]);
  for(const draw::AtlasRect& rect : hotbarSprites) {
   draw::appendAtlasQuad(tess, rect.x, rect.y, rect.u, rect.v, rect.w, rect.h, kHudZ);
  }
  tess.draw();
 }
 minecraft->textureManager.bindTexture(minecraft->textureManager.getTextureId("/gui/icons.png"));
  {
   const render::RenderPassScope passScope(render::RenderType::guiTextured());
   const float* c = core::constColor();
   render::Tessellator& tess = render::Tessellator::INSTANCE;
   tess.startQuads();
   tess.color(c[0], c[1], c[2], c[3]);
   draw::appendAtlasQuad(tess, scaledWidth / 2 - 7, scaledHeight / 2 - 7, 0, 0, 16, 16, kHudZ);
   tess.draw();
  }
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
  {
   const render::RenderPassScope passScope(render::RenderType::guiTextured());
   const float* c = core::constColor();
   render::Tessellator& tess = render::Tessellator::INSTANCE;
   tess.startQuads();
   tess.color(c[0], c[1], c[2], c[3]);
   for(const draw::AtlasRect& rect : statusSprites) {
    draw::appendAtlasQuad(tess, rect.x, rect.y, rect.u, rect.v, rect.w, rect.h, kHudZ);
   }
   tess.draw();
  }
 }
 core::disableBlend();
 for(int slot = 0; slot < 9; ++slot) {
  const int x = scaledWidth / 2 - 90 + slot * 20 + 2;
  const int y = scaledHeight - 16 - 3;
  renderHotbarItem(slot, x, y, tickDelta);
 }
  core::setLightingEnabled(false);
  if(player.getSleepTimer() > 0) {
   const int sleepTimer = player.getSleepTimer();
  float fade = static_cast<float>(sleepTimer) / 100.0f;
  if(fade > 1.0f) {
   fade = 1.0f - static_cast<float>(sleepTimer - 100) / 10.0f;
  }
  const int color = (static_cast<int>(220.0f * fade) << 24) | 0x101020;
  {
   const render::RenderPassScope passScope(render::RenderType::gui());
   draw::coloredQuad(render::Tessellator::INSTANCE,
                     scaledWidth,
                     scaledHeight,
                     0,
                     0,
                     static_cast<int>(static_cast<std::uint32_t>(color) & 0x00FFFFFFU),
                     static_cast<int>((static_cast<std::uint32_t>(color) >> 24U) & 0xFFU),
                     kHudZ);
  }
 }
 if(minecraft->options.debugHud) {
  renderDebugHud(textRenderer, player);
 }
 if(overlayRemaining > 0 && !overlayMessage.empty()) {
  renderRecordOverlay(textRenderer, tickDelta, scaledWidth, scaledHeight);
 }
 renderChat(textRenderer, screenOpen, scaledWidth, scaledHeight);
 core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
 mod::ScreenRegionEvent hudRegion;
 hudRegion.screenId = mod::screen_ids::kHud;
 hudRegion.region = mod::screen_regions::kHud;
 hudRegion.phase = mod::ScreenRegionPhase::Render;
 hudRegion.mouseX = mouseX;
 hudRegion.mouseY = mouseY;
 hudRegion.x = 0;
 hudRegion.y = 0;
 hudRegion.width = scaledWidth;
 hudRegion.height = scaledHeight;
 net::minecraft::mod::runtime::luaHookScreenRegion(hudRegion);
 core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
}
} // namespace net::minecraft::client::gui::hud
