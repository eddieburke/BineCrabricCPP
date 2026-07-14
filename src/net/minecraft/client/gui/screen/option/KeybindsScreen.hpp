#pragma once
#include <functional>
#include <memory>
#include <string>
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
namespace net::minecraft::client::gui::screen::option {
namespace client_option = net::minecraft::client::option;
class KeybindsScreen : public screen::Screen {
public:
  using ParentFactory = std::function<std::unique_ptr<screen::Screen>()>;
  KeybindsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions);
  void init() override;
  void render(int mouseX, int mouseY, float tickDelta) override;
  void keyPressed(char character, int keyCode) override;
  [[nodiscard]] std::string_view getScreenUiId() const override {
    return net::minecraft::mod::screen_ids::kKeybinds;
  }

private:
  void selectKeybind(int index);
  [[nodiscard]] int controlsListX() const {
    return width() / 2 - 155;
  }
  ParentFactory parentFactory_;
  client_option::GameOptions* gameOptions_ = nullptr;
  std::string title_;
  int selectedKeyBinding_ = -1;
};
} // namespace net::minecraft::client::gui::screen::option
