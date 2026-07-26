#pragma once
#include <array>
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/OptionSpec.hpp"
namespace net::minecraft::client::gui::screen::option {
namespace options_screen {
extern std::array<net::minecraft::client::option::OptionSpec, 8> kSpecs;
}
class OptionsScreen : public screen::Screen {
 public:
 OptionsScreen(screen::ScreenFactory parentFactory, net::minecraft::client::option::GameOptions* options);
 void init() override;
 void render(int mouseX, int mouseY, float tickDelta) override;
 [[nodiscard]] std::string_view getScreenUiId() const override {
  return net::minecraft::mod::screen_ids::kOptions;
 }

 private:
 screen::ScreenFactory parentFactory_;
 net::minecraft::client::option::GameOptions* options_ = nullptr;
 std::string title_;
};
} // namespace net::minecraft::client::gui::screen::option
