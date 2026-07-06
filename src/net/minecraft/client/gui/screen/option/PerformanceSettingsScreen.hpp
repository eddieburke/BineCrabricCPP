#pragma once
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/OptionSpec.hpp"
#include <array>
#include <functional>
#include <memory>
namespace net::minecraft::client::gui::screen::option {
namespace performance_screen {
extern std::array<net::minecraft::client::option::OptionSpec, 10> kSpecs;
}
std::unique_ptr<screen::Screen>
makePerformanceSettingsScreen(std::function<std::unique_ptr<screen::Screen>()> parentFactory,
                              net::minecraft::client::option::GameOptions* gameOptions);
} // namespace net::minecraft::client::gui::screen::option
