#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/mod/ScreenUi.hpp"
namespace net::minecraft::client::gui::screen::option {
class ModSettingsScreen : public screen::Screen {
public:
  using ParentFactory = std::function<std::unique_ptr<screen::Screen>()>;
  explicit ModSettingsScreen(ParentFactory parentFactory = nullptr, bool openModPages = false);
  [[nodiscard]] ParentFactory modPagesFactory() const;
  void init() override;
  void render(int mouseX, int mouseY, float tickDelta) override;
  void mouseReleased(int mouseX, int mouseY, int button) override;
  void keyPressed(char character, int keyCode) override;
  void mouseScrolled(int mouseX, int mouseY, int delta) override;
  [[nodiscard]] std::string_view getScreenUiId() const override {
    return net::minecraft::mod::screen_ids::kModSettings;
  }

private:
  enum class Page {
    Settings,
    ModPages,
  };
  struct SettingWidget {
    std::string modId;
    std::string key;
    int widgetIndex = 0;
    int contentY = 0;
  };
  struct KeybindWidget {
    std::string kbId;
    int widgetIndex = 0;
    int contentY = 0;
  };
  struct SectionHeader {
    std::string text;
    int contentY = 0;
  };
  struct InjectedWidget {
    int widgetIndex = 0;
    int contentY = 0;
  };
  struct InjectedPage {
    std::string text;
    std::function<void()> onClick;
  };
  void rebuildLayout();
  void updateListLayout();
  void scrollBy(int amount);
  ParentFactory parentFactory_;
  bool openModPages_ = false;
  std::string title_;
  std::vector<SettingWidget> settingWidgets_;
  std::vector<KeybindWidget> keybindWidgets_;
  std::vector<SectionHeader> sectionHeaders_;
  std::vector<InjectedWidget> injectedWidgets_;
  std::vector<InjectedPage> injectedPages_;
  bool pendingPageRebuild_ = false;
  Page page_ = Page::Settings;
  int selectedKeybindIndex_ = -1;
  int listTop_ = 42;
  int listBottom_ = 42;
  int contentHeight_ = 0;
  int scrollOffset_ = 0;
  int maxScroll_ = 0;
};
}
