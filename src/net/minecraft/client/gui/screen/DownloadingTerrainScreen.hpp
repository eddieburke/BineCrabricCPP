#pragma once
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/multiplayer/ClientNetworkHandler.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
namespace net::minecraft::client::gui::screen {
class DownloadingTerrainScreen : public Screen {
 public:
  explicit DownloadingTerrainScreen(multiplayer::ClientNetworkHandler* networkHandler) {
   (void)networkHandler;
  }
 void keyPressed(char character, int keyCode) override {
  (void)character;
  (void)keyCode;
 }
  void init() override {
   buttons_.clear();
  }
  void render(int mouseX, int mouseY, float tickDelta) override {
   renderBackgroundTexture(0);
   if(textRenderer_ != nullptr) {
    textRenderer_->drawCenteredWithShadow(resource::language::I18n::getTranslation("multiplayer.downloadingTerrain"),
                                          width_ / 2,
                                          height_ / 2 - 50,
                                          0xFFFFFF);
   }
   Screen::render(mouseX, mouseY, tickDelta);
  }
  [[nodiscard]] std::string_view getScreenUiId() const override {
   return net::minecraft::mod::screen_ids::kDownloadingTerrain;
  }
};
} // namespace net::minecraft::client::gui::screen
