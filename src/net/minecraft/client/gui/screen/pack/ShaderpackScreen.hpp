#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "net/minecraft/client/gui/layout/OptionsListScroll.hpp"
#include "net/minecraft/client/gui/screen/Screen.hpp"
namespace net::minecraft::client::gui::screen::pack {
class ShaderpackScreen : public screen::Screen {
 public:
 using ParentFactory = std::function<std::unique_ptr<screen::Screen>()>;
 explicit ShaderpackScreen(ParentFactory parentFactory = {});
 void init() override;
 void render(int mouseX, int mouseY, float tickDelta) override;
 void mouseScrolled(int mouseX, int mouseY, int delta) override;
 void mouseClicked(int mouseX, int mouseY, int button) override;
 void mouseReleased(int mouseX, int mouseY, int button) override;
 void keyPressed(char character, int keyCode) override;

 private:
 void rebuildLayout();
 void updateListLayout();
 ParentFactory parentFactory_;
 std::string title_;
 // Iris screen.NAME page; empty = root `screen=` layout.
 std::string currentPage_;
 layout::OptionsListScroll scroll_;
};
} // namespace net::minecraft::client::gui::screen::pack
