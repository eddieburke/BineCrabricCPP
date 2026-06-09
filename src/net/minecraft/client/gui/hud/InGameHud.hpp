#pragma once

#include "net/minecraft/client/gui/DrawContext.hpp"
#include "net/minecraft/client/gui/hud/ChatHudLine.hpp"

#include <random>
#include <string>
#include <vector>

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

class InGameHud : public gui::DrawContext {
public:
    void setClient(Minecraft* client) { minecraft = client; }

    void tick();
    void clearChat();
    void addChatMessage(const std::string& message);
    void addTranslatedChatMessage(const std::string& text);
    void setRecordPlayingOverlay(const std::string& record);

    [[nodiscard]] const std::vector<ChatHudLine>& chatLines() const { return messages; }
    [[nodiscard]] const std::string& overlay() const { return overlayMessage; }
    [[nodiscard]] int overlayTicks() const { return overlayRemaining; }
    [[nodiscard]] bool isOverlayTinted() const { return overlayTinted; }

    void render(float tickDelta, bool screenOpen, int mouseX, int mouseY);

    std::string selectedName {};
    int ticks = 0;
    std::string overlayMessage {};
    int overlayRemaining = 0;
    bool overlayTinted = false;
    float progress = 0.0f;
    float vignetteDarkness = 1.0f;

private:
    void renderHotbarItem(int slot, int x, int y, float tickDelta);
    void renderVignette(float brightness, int width, int height);
    void renderPumpkinOverlay(int width, int height);
    void renderPortalOverlay(float strength, int width, int height);
    void renderDebugHud(font::TextRenderer& textRenderer, int scaledWidth, const entity::player::PlayerEntity& player);
    void renderRecordOverlay(font::TextRenderer& textRenderer, float tickDelta, int scaledWidth, int scaledHeight);
    void renderChat(font::TextRenderer& textRenderer, bool chatOpen, int scaledWidth, int scaledHeight);

    Minecraft* minecraft = nullptr;
    std::vector<ChatHudLine> messages {};
    std::mt19937 random {0xB17A1U};
};

} // namespace net::minecraft::client::gui::hud
