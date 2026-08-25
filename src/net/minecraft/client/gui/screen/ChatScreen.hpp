#pragma once
#include <string>
#include "net/minecraft/client/gui/screen/Screen.hpp"
namespace net::minecraft::client::gui::screen {
class ChatScreen : public Screen {
 public:
 void init() override;
 void removed() override;
 void tick() override;
 void render(int mouseX, int mouseY, float tickDelta) override;
 void keyPressed(char character, int keyCode) override;
 void mouseClicked(int mouseX, int mouseY, int button) override;
 void mouseScrolled(int mouseX, int mouseY, int delta) override;
 [[nodiscard]] std::string_view getScreenUiId() const override {
  return net::minecraft::mod::screen_ids::kChat;
 }

 protected:
 [[nodiscard]] std::string trimmedText() const;
 void sendCurrentText();
 std::string text_;

 private:
 void recallHistory(int direction);
 int focusedTicks_ = 0;
 int historyOffset_ = -1;
 std::string pendingText_;
};
} // namespace net::minecraft::client::gui::screen
