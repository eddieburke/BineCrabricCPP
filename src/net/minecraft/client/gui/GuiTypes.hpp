#pragma once

// M2 single-include contract: include this header at most once per file that
// needs net::minecraft gui type aliases. Do not sandwich includes between
// other headers — include GuiTypes.hpp once in the include block.

// Forward declarations — canonical namespaces only, no concrete includes.

namespace net::minecraft::client::gui {
class ParticlesGui;
class GuiParticle;
} // namespace net::minecraft::client::gui

namespace net::minecraft::client::gui::screen {
class AchievementsScreen;
class StatsScreen;
class DisconnectedScreen;
class DownloadingTerrainScreen;
} // namespace net::minecraft::client::gui::screen

namespace net::minecraft::client::gui::screen::ingame {
class DispenserScreen;
class CraftingScreen;
class FurnaceScreen;
class DoubleChestScreen;
class HandledScreen;
class SignEditScreen;
} // namespace net::minecraft::client::gui::screen::ingame

namespace net::minecraft::client::gui::screen::option {
class KeybindsScreen;
class VideoOptionsScreen;
} // namespace net::minecraft::client::gui::screen::option

namespace net::minecraft::client::gui::widget {
class SliderWidget;
class OptionButtonWidget;
} // namespace net::minecraft::client::gui::widget

// Aliases into net::minecraft flat namespace.

namespace net::minecraft {

using ParticlesGui    = gui::ParticlesGui;
using GuiParticle     = gui::GuiParticle;

using AchievementsScreen    = gui::screen::AchievementsScreen;
using StatsScreen           = gui::screen::StatsScreen;
using DisconnectedScreen    = gui::screen::DisconnectedScreen;
using DownloadingTerrainScreen = gui::screen::DownloadingTerrainScreen;

using DispenserScreen   = gui::screen::ingame::DispenserScreen;
using CraftingScreen    = gui::screen::ingame::CraftingScreen;
using FurnaceScreen     = gui::screen::ingame::FurnaceScreen;
using DoubleChestScreen = gui::screen::ingame::DoubleChestScreen;
using HandledScreen     = gui::screen::ingame::HandledScreen;
using SignEditScreen    = gui::screen::ingame::SignEditScreen;

using KeybindsScreen    = gui::screen::option::KeybindsScreen;
using VideoOptionsScreen = gui::screen::option::VideoOptionsScreen;

using SliderWidget       = gui::widget::SliderWidget;
using OptionButtonWidget = gui::widget::OptionButtonWidget;

} // namespace net::minecraft
