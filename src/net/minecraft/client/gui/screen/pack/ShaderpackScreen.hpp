#pragma once
#include <functional>
#include <memory>
#include <string>
#include "net/minecraft/client/gui/screen/Screen.hpp"
namespace net::minecraft::client::gui::screen::pack {
class ShaderpackScreen : public screen::Screen {
 public:
 using ParentFactory = std::function<std::unique_ptr<screen::Screen>()>;
 explicit ShaderpackScreen(ParentFactory parentFactory = {});
 void init() override;
 void render(int mouseX, int mouseY, float tickDelta) override;

 private:
 ParentFactory parentFactory_;
 std::string title_;
};
} // namespace net::minecraft::client::gui::screen::pack
