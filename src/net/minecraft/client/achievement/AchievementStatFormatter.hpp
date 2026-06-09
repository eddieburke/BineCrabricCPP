#pragma once

#include <string>

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft::client::achievement {

/// Formats achievement stat strings with the inventory key name (Java `AchievementStatFormatter`).
/// Anchor: Minecraft.cpp L140–154.
class AchievementStatFormatter {
public:
    explicit AchievementStatFormatter(Minecraft* client);
    [[nodiscard]] std::string format(const std::string& text) const;

private:
    Minecraft* client_ = nullptr;
};

} // namespace net::minecraft::client::achievement
