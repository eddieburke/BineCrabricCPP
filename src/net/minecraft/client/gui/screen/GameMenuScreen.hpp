#pragma once
#include "net/minecraft/client/gui/screen/Screen.hpp"
namespace net::minecraft::client::gui::screen {
class GameMenuScreen : public Screen {
public:
  void init() override;
  void tick() override;
  void render(int mouseX, int mouseY, float tickDelta) override;

private:
  void saveAndQuit();
  void resumeGame();
  void openLanMenu();
  void openLanInfo();
  void openOptions();
  void openAchievements();
  void openStats();
  int saveStep_ = 0;
  int ticks_ = 0;
  bool savingLevel_ = true;
};
} // namespace net::minecraft::client::gui::screen
