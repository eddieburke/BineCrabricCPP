#pragma once
#include <array>
#include <functional>
#include <memory>
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/option/OptionSpec.hpp"
namespace net::minecraft::client::gui::screen::option {
namespace lod_screen {
extern std::array<net::minecraft::client::option::OptionSpec, 4> kSpecs;
}
std::unique_ptr<screen::Screen> makeLodSettingsScreen(
    std::function<std::unique_ptr<screen::Screen>()> parentFactory,
    net::minecraft::client::option::GameOptions* gameOptions);
}
