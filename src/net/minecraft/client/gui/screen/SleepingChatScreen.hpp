#pragma once
#include "net/minecraft/client/gui/screen/ChatScreen.hpp"
namespace net::minecraft::client::gui::screen {
class SleepingChatScreen : public ChatScreen {
public:
  void init() override;
  void removed() override;
  void keyPressed(char character, int keyCode) override;
  [[nodiscard]] std::string_view getScreenUiId() const override {
    return net::minecraft::mod::screen_ids::kSleepingChat;
  }

private:
  void stopSleeping();
};
} // namespace net::minecraft::client::gui::screen
