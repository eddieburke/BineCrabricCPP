#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "net/minecraft/client/gui/layout/OptionsListScroll.hpp"
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/mod/ScreenUi.hpp"
namespace net::minecraft::client::gui::screen::option {
class ModSettingsScreen : public screen::Screen {
 public:
 using ParentFactory = std::function<std::unique_ptr<screen::Screen>()>;
 explicit ModSettingsScreen(ParentFactory parentFactory = nullptr);
 [[nodiscard]] ParentFactory modPagesFactory() const;
 void init() override;
 void render(int mouseX, int mouseY, float tickDelta) override;
 void mouseReleased(int mouseX, int mouseY, int button) override;
 void mouseClicked(int mouseX, int mouseY, int button) override;
 void keyPressed(char character, int keyCode) override;
 void mouseScrolled(int mouseX, int mouseY, int delta) override;
 [[nodiscard]] std::string_view getScreenUiId() const override {
  return net::minecraft::mod::screen_ids::kModSettings;
 }
 [[nodiscard]] screen::ScreenFactory getReopenFactory() const override {
  return modPagesFactory();
 }

 private:
 struct SettingWidget {
  std::string modId;
  std::string key;
  int widgetIndex = 0;
 };
 struct KeybindWidget {
  std::string kbId;
  int widgetIndex = 0;
 };
 void rebuildLayout();
 void updateListLayout();
 ParentFactory parentFactory_;
 std::string title_;
 std::vector<SettingWidget> settingWidgets_;
 std::vector<KeybindWidget> keybindWidgets_;
 layout::OptionsListScroll scroll_;
 int selectedKeybindIndex_ = -1;
};
} // namespace net::minecraft::client::gui::screen::option
