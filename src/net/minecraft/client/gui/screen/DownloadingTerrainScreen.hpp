#pragma once
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/network/packet/ConnectionPackets.hpp"

namespace net::minecraft::client::gui::screen {
class DownloadingTerrainScreen : public Screen {
   public:
    explicit DownloadingTerrainScreen(multiplayer::ClientNetworkHandler* networkHandler)
        : networkHandler_(networkHandler) {
    }

    void keyPressed(char character, int keyCode) override {
        (void) character;
        (void) keyCode;
    }

    void init() override {
        buttons_.clear();
    }

    void tick() override {
        ++ticks_;
        if (ticks_ % 20 == 0 && networkHandler_ != nullptr) {
            networkHandler_->sendPacket(KeepAlivePacket{});
        }
        if (networkHandler_ != nullptr) {
            networkHandler_->tick();
        }
    }

    void render(int mouseX, int mouseY, float tickDelta) override {
        renderBackgroundTexture(0);
        if (textRenderer_ != nullptr) {
            drawCenteredTextWithShadow(*textRenderer_,
                                       resource::language::I18n::getTranslation("multiplayer.downloadingTerrain"),
                                       width_ / 2,
                                       height_ / 2 - 50,
                                       0xFFFFFF);
        }
        Screen::render(mouseX, mouseY, tickDelta);
    }

    [[nodiscard]] std::string_view getScreenUiId() const override {
        return net::minecraft::mod::screen_ids::kDownloadingTerrain;
    }

   private:
    multiplayer::ClientNetworkHandler* networkHandler_ = nullptr;
    int ticks_ = 0;
};
}  // namespace net::minecraft::client::gui::screen
