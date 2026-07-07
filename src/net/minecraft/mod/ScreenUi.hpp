#pragma once
#include "net/minecraft/mod/HookBus.hpp"
#include <functional>
#include <string>
#include <string_view>
namespace net::minecraft::client::gui::screen {
class Screen;
}
namespace net::minecraft::client::gui::widget {
class ActionButtonWidget;
}
namespace net::minecraft::mod {
struct ScreenUiContext {
  client::gui::screen::Screen* screen = nullptr;
  std::string_view screenId;
  std::string_view region;
  int* stackedButtonY = nullptr;
  [[nodiscard]] client::gui::widget::ActionButtonWidget& addCenteredButton(int y, std::string text,
                                                                           std::function<void()> onClick) const;
  [[nodiscard]] client::gui::widget::ActionButtonWidget& addButton(int x, int y, int width, int height, std::string text,
                                                                   std::function<void()> onClick) const;
  [[nodiscard]] client::gui::widget::ActionButtonWidget& addStackedCenteredButton(std::string text,
                                                                                  std::function<void()> onClick) const;
};
struct ScreenUiEvent {
  ScreenUiContext* context = nullptr;
};
enum class ScreenRegionPhase {
  Render,
  MouseClick,
  MouseScroll,
};
struct ScreenRegionEvent {
  client::gui::screen::Screen* screen = nullptr;
  std::string_view screenId;
  std::string_view region;
  ScreenRegionPhase phase = ScreenRegionPhase::Render;
  int mouseX = 0;
  int mouseY = 0;
  int button = 0;
  int scrollDelta = 0;
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
  bool handled = false;
};
namespace screen_ids {
inline constexpr std::string_view kCreateWorld = "minecraft:create_world";
inline constexpr std::string_view kInventory = "minecraft:inventory";
inline constexpr std::string_view kDetailSettings = "minecraft:detail_settings";
inline constexpr std::string_view kWorldSettings = "minecraft:world_settings";
} // namespace screen_ids
namespace screen_regions {
inline constexpr std::string_view kFooter = "footer";
inline constexpr std::string_view kSidePanel = "side_panel";
// Published by Screen::init() for *every* screen after it builds its widgets, so
// a mod can add buttons/behaviour to any GUI by matching context->screenId.
// Screens without an explicit getScreenUiId() expose their typeid name as the id.
inline constexpr std::string_view kScreen = "screen";
} // namespace screen_regions
} // namespace net::minecraft::mod
