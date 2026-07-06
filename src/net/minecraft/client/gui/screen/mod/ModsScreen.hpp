#pragma once
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/gui/widget/EntryListWidget.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include <filesystem>
#include <memory>
#include <vector>
namespace net::minecraft::client::gui::screen::mod {
class ModsScreen : public screen::Screen {
public:
  explicit ModsScreen(screen::ScreenFactory parentFactory = {});
  ~ModsScreen() override;
  void init() override;
  void render(int mouseX, int mouseY, float tickDelta) override;
  void tick() override;

private:
  class ModListWidget;
  void refreshMods();
  void toggleSelected();
  void updateToggleButton();
  void openModsFolder();
  void openTexturePacks();
  screen::ScreenFactory parentFactory_;
  std::unique_ptr<ModListWidget> modList_;
  std::vector<net::minecraft::mod::runtime::ModPackage> mods_;
  std::filesystem::path modsDir_;
  widget::ActionButtonWidget* toggleButton_ = nullptr;
  int selectedIndex_ = -1;
  std::string footerText_;
};
} // namespace net::minecraft::client::gui::screen::mod
