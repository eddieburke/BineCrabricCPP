#pragma once
#include <optional>
#include <random>
#include <string>
#include <vector>
#include "net/minecraft/client/gui/hud/ChatHudLine.hpp"
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::entity::player {
class PlayerEntity;
}
namespace net::minecraft::client::font {
class TextRenderer;
}
namespace net::minecraft::client::gui::hud {
class InGameHud {
 public:
 void setClient(Minecraft* client) {
  minecraft = client;
 }
 void tick();
 void clearChat();
 void addChatMessage(const std::string& message);
 void addTranslatedChatMessage(const std::string& text);
 void addSentChatMessage(std::string message);
 void scrollChat(int lines);
 void resetChatScroll();
 void setRecordPlayingOverlay(const std::string& record);
 [[nodiscard]] const std::vector<ChatHudLine>& chatLines() const {
  return messages;
 }
 [[nodiscard]] const std::vector<std::string>& sentChatMessages() const {
  return sentMessages_;
 }
 [[nodiscard]] std::string chatLineAt(int mouseX, int mouseY, int scaledHeight) const;
 [[nodiscard]] std::string chatLinkAt(int mouseX, int mouseY, int scaledHeight) const;
 [[nodiscard]] const std::string& overlay() const {
  return overlayMessage;
 }
 [[nodiscard]] int overlayTicks() const {
  return overlayRemaining;
 }
 [[nodiscard]] bool isOverlayTinted() const {
  return overlayTinted;
 }
 void render(float tickDelta, bool screenOpen, int mouseX, int mouseY);
 std::string selectedName{};
 int ticks = 0;
 std::string overlayMessage{};
 int overlayRemaining = 0;
 bool overlayTinted = false;
 float progress = 0.0f;
 float vignetteDarkness = 1.0f;

 private:
 void renderHotbarItem(int slot, int x, int y, float tickDelta);
 void renderVignette(float brightness, int width, int height);
 void renderPumpkinOverlay(int width, int height);
 void renderPortalOverlay(float strength, int width, int height);
 void renderDebugHud(font::TextRenderer& textRenderer, const entity::player::PlayerEntity& player);
 void renderRecordOverlay(font::TextRenderer& textRenderer, float tickDelta, int scaledWidth, int scaledHeight);
 void renderChat(font::TextRenderer& textRenderer, bool chatOpen, int scaledWidth, int scaledHeight);
 [[nodiscard]] std::optional<std::size_t> chatLineIndexAt(int mouseX, int mouseY, int scaledHeight) const;
 Minecraft* minecraft = nullptr;
 std::vector<ChatHudLine> messages{};
 std::vector<std::string> sentMessages_{};
 int chatScroll_ = 0;
 std::mt19937 random{0xB17A1U};
};
} // namespace net::minecraft::client::gui::hud
