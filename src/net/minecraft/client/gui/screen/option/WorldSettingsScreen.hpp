#pragma once
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/OptionSpec.hpp"
namespace net::minecraft::client::gui::screen::option {
namespace client_option = net::minecraft::client::option;
namespace world_screen {
extern std::array<net::minecraft::client::option::OptionSpec, 4> kSpecs;
}
class WorldSettingsScreen : public screen::Screen {
 public:
 using ParentFactory = std::function<std::unique_ptr<screen::Screen>()>;
 WorldSettingsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions);
 void init() override;
 void render(int mouseX, int mouseY, float tickDelta) override;
 [[nodiscard]] std::string_view getScreenUiId() const override;

 private:
 ParentFactory parentFactory_;
 client_option::GameOptions* gameOptions_ = nullptr;
 std::string title_;
};
} // namespace net::minecraft::client::gui::screen::option
