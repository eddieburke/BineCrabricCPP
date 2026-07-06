#pragma once
#include "net/minecraft/client/gui/DrawContext.hpp"
#include <cstdint>
#include <string>
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::gui::hud::toast {
class AchievementToast : public gui::DrawContext {
public:
  void setClient(Minecraft* client) {
    client_ = client;
  }
  void set(int achievementStatId);
  void setTutorial(int achievementStatId);
  void clear();
  [[nodiscard]] const std::string& current() const {
    return current_;
  }
  [[nodiscard]] const std::string& tutorial() const {
    return tutorial_;
  }
  [[nodiscard]] bool empty() const {
    return current_.empty() && tutorial_.empty();
  }
  void tick();

private:
  void renderOverlay();
  Minecraft* client_ = nullptr;
  std::string title_{};
  std::string description_{};
  std::string current_{};
  std::string tutorial_{};
  std::int64_t startTime_ = 0;
  int achievementStatId_ = -1;
  bool tutorialMode_ = false;
  bool active_ = false;
};
} // namespace net::minecraft::client::gui::hud::toast
