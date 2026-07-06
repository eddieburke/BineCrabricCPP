#pragma once
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/gui/widget/EntryListWidget.hpp"
#include <memory>
namespace net::minecraft::stat {
class PlayerStats;
}
namespace net::minecraft::client::gui::screen {
class StatsScreen : public Screen {
public:
  StatsScreen(ScreenFactory parentFactory, stat::PlayerStats* stats);
  void init() override;
  void render(int mouseX, int mouseY, float tickDelta) override;
  void createButtons();
  void renderIcon(int x, int y, int u = 0, int v = 0);
  void renderItemIcon(int x, int y, int itemOrBlockId);

private:
  ScreenFactory parentFactory_;
  stat::PlayerStats* stats_ = nullptr;
  std::string title_;
  std::unique_ptr<widget::EntryListWidget> generalStats_;
  std::unique_ptr<widget::EntryListWidget> itemStats_;
  std::unique_ptr<widget::EntryListWidget> blockStats_;
  widget::EntryListWidget* selectedStatsList_ = nullptr;
};
} // namespace net::minecraft::client::gui::screen
