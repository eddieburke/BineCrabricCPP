#pragma once
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include <functional>
#include <memory>
#include <string>
namespace net::minecraft::client::gui::screen::option {
namespace client_option = net::minecraft::client::option;
class VideoOptionsScreen : public screen::Screen {
public:
  using ParentFactory = std::function<std::unique_ptr<screen::Screen>()>;
  VideoOptionsScreen(ParentFactory parentFactory, client_option::GameOptions* options);
  void init() override;
  void render(int mouseX, int mouseY, float tickDelta) override;
  [[nodiscard]] std::string_view getScreenUiId() const override { return net::minecraft::mod::screen_ids::kVideoOptions; }

 private:
  ParentFactory parentFactory_;
  client_option::GameOptions* options_ = nullptr;
  std::string title_;
};
} // namespace net::minecraft::client::gui::screen::option
