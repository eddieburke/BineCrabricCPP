#pragma once

#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/OptionSpec.hpp"

#include <array>
#include <functional>
#include <memory>

namespace net::minecraft::client::gui::screen::option {
namespace quality_screen {
extern std::array<net::minecraft::client::option::OptionSpec, 9> kSpecs;
}

std::unique_ptr<screen::Screen> makeQualitySettingsScreen(
    std::function<std::unique_ptr<screen::Screen>()> parentFactory,
    net::minecraft::client::option::GameOptions* gameOptions);

} // namespace
