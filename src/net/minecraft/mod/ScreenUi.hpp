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
// Stable, first-party screen ids. Every GUI that a mod might want to inject into or
// target exposes one of these from Screen::getScreenUiId(). Screens that do not
// override getScreenUiId() still publish under their (mangled) typeid name, but
// first-party screens should always declare a friendly id so mods have a stable target.
namespace screen_ids {
inline constexpr std::string_view kLogin = "minecraft:login";
inline constexpr std::string_view kTitle = "minecraft:title";
inline constexpr std::string_view kGameMenu = "minecraft:game_menu";
inline constexpr std::string_view kMultiplayer = "minecraft:multiplayer";
inline constexpr std::string_view kConnect = "minecraft:connect";
inline constexpr std::string_view kDisconnected = "minecraft:disconnected";
inline constexpr std::string_view kDownloadingTerrain = "minecraft:downloading_terrain";
inline constexpr std::string_view kDeath = "minecraft:death";
inline constexpr std::string_view kChat = "minecraft:chat";
inline constexpr std::string_view kSleepingChat = "minecraft:sleeping_chat";
inline constexpr std::string_view kConfirm = "minecraft:confirm";
inline constexpr std::string_view kCreateWorld = "minecraft:create_world";
inline constexpr std::string_view kSelectWorld = "minecraft:select_world";
inline constexpr std::string_view kEditWorld = "minecraft:edit_world";
inline constexpr std::string_view kWorldSettings = "minecraft:world_settings";
inline constexpr std::string_view kWorldSaveConflict = "minecraft:world_save_conflict";
inline constexpr std::string_view kInventory = "minecraft:inventory";
inline constexpr std::string_view kCrafting = "minecraft:crafting";
inline constexpr std::string_view kDispenser = "minecraft:dispenser";
inline constexpr std::string_view kDoubleChest = "minecraft:double_chest";
inline constexpr std::string_view kFurnace = "minecraft:furnace";
inline constexpr std::string_view kSignEdit = "minecraft:sign_edit";
inline constexpr std::string_view kOptions = "minecraft:options";
inline constexpr std::string_view kVideoOptions = "minecraft:video_options";
inline constexpr std::string_view kDetailSettings = "minecraft:detail_settings";
inline constexpr std::string_view kKeybinds = "minecraft:keybinds";
inline constexpr std::string_view kMods = "minecraft:mods";
inline constexpr std::string_view kAchievements = "minecraft:achievements";
inline constexpr std::string_view kStats = "minecraft:stats";
inline constexpr std::string_view kLan = "minecraft:lan";
inline constexpr std::string_view kLanInfo = "minecraft:lan_info";
inline constexpr std::string_view kServerModDownload = "minecraft:server_mod_download";
inline constexpr std::string_view kFatalError = "minecraft:fatal_error";
inline constexpr std::string_view kOutOfMemory = "minecraft:out_of_memory";
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
