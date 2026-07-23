#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "net/minecraft/client/gui/screen/Screen.hpp"
namespace net::minecraft::client::gui::screen::pack {
class ShaderpackScreen : public screen::Screen {
 public:
 using ParentFactory = std::function<std::unique_ptr<screen::Screen>()>;
 explicit ShaderpackScreen(ParentFactory parentFactory = {});
 void init() override;
 void render(int mouseX, int mouseY, float tickDelta) override;
 void mouseScrolled(int mouseX, int mouseY, int delta) override;
 void keyPressed(char character, int keyCode) override;

 private:
 struct SettingWidgetEntry {
  std::string key;
  int widgetIndex = -1;
  int contentY = 0;
 };
 struct HeaderEntry {
  std::string text;
  int contentY = 0;
 };
 void rebuildLayout();
 void updateListLayout();
 void scrollBy(int amount);
 ParentFactory parentFactory_;
 std::string title_;
 std::vector<SettingWidgetEntry> settingWidgets_;
 std::vector<HeaderEntry> sectionHeaders_;
 int listTop_ = 42;
 int listBottom_ = 200;
 int contentHeight_ = 0;
 int scrollOffset_ = 0;
 int maxScroll_ = 0;
};
} // namespace net::minecraft::client::gui::screen::pack
