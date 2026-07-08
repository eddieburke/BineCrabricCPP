#pragma once
#include <string>

namespace net::minecraft::client::gui::hud {
struct ChatHudLine {
    explicit ChatHudLine(std::string textIn = {}) : text(std::move(textIn)) {
    }

    std::string text{};
    int age = 0;
};
}  // namespace net::minecraft::client::gui::hud
