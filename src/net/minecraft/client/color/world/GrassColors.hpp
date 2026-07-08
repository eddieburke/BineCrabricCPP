#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <utility>

namespace net::minecraft::client::color::world {
class GrassColors {
   public:
    static void setColorMap(std::span<const int> colorMap) {
        auto& map = mutableColorMap();
        const std::size_t count = std::min(map.size(), colorMap.size());
        for (std::size_t i = 0; i < count; ++i) {
            map[i] = colorMap[i];
        }
    }

    // World-profile themes (or any mod) register a tint override that remaps the biome-derived
    // base color. Applied here so every consumer (chunk mesh bake, map widget, particles) is
    // themed uniformly; engine stays mod-agnostic by talking through std::function only.
    static void setColorOverride(std::function<int(int)> override) {
        colorOverride() = std::move(override);
    }

    [[nodiscard]] static int getColor(double temperature, double humidity) {
        const int x = static_cast<int>((1.0 - temperature) * 255.0);
        humidity *= temperature;
        const int y = static_cast<int>((1.0 - humidity) * 255.0);
        const int base = mutableColorMap()[static_cast<std::size_t>((y << 8) | x)];
        const auto& override = colorOverride();
        return override ? override(base) : base;
    }

   private:
    [[nodiscard]] static std::array<int, 65536>& mutableColorMap() {
        static std::array<int, 65536> colorMap{};
        return colorMap;
    }

    [[nodiscard]] static std::function<int(int)>& colorOverride() {
        static std::function<int(int)> override;
        return override;
    }
};
}  // namespace net::minecraft::client::color::world
