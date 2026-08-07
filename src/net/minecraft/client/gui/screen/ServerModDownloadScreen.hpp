#pragma once
#include <string>
#include <vector>
#include "net/minecraft/client/gui/screen/Screen.hpp"
namespace net::minecraft::client::multiplayer {
class ClientLoginNetworkHandler;
}
namespace net::minecraft::client::gui::screen {
class ServerModDownloadScreen : public Screen {
 public:
 ServerModDownloadScreen(multiplayer::ClientLoginNetworkHandler* handler, std::vector<std::string> missingMods);
 void init() override;
 void tick() override;
 void render(int mouseX, int mouseY, float tickDelta) override;
 void keyPressed(char character, int keyCode) override;
 [[nodiscard]] std::string_view getScreenUiId() const override {
  return net::minecraft::mod::screen_ids::kServerModDownload;
 }

 private:
 void accept();
 void cancel();
 multiplayer::ClientLoginNetworkHandler* handler_ = nullptr;
 std::vector<std::string> missingMods_;
 std::string status_;
 bool busy_ = false;
};
} // namespace net::minecraft::client::gui::screen
