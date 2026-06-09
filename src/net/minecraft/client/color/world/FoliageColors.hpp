#pragma once

#include <algorithm>
#include <array>
#include <span>

namespace net::minecraft::client::color::world {

class FoliageColors {
public:
    static void setColorMap(std::span<const int> colorMap)
    {
        auto& map = mutableColorMap();
        const std::size_t count = std::min(map.size(), colorMap.size());
        for (std::size_t i = 0; i < count; ++i) {
            map[i] = colorMap[i];
        }
    }

    [[nodiscard]] static int getColor(double temperature, double humidity)
    {
        const int x = static_cast<int>((1.0 - temperature) * 255.0);
        humidity *= temperature;
        const int y = static_cast<int>((1.0 - humidity) * 255.0);
        return mutableColorMap()[static_cast<std::size_t>((y << 8) | x)];
    }

    [[nodiscard]] static constexpr int getSpruceColor() noexcept
    {
        return 0x619961;
    }

    [[nodiscard]] static constexpr int getBirchColor() noexcept
    {
        return 8431445;
    }

    [[nodiscard]] static constexpr int getDefaultColor() noexcept
    {
        return 4764952;
    }

private:
    [[nodiscard]] static std::array<int, 65536>& mutableColorMap()
    {
        static std::array<int, 65536> colorMap {};
        return colorMap;
    }
};

} // namespace net::minecraft::client::color::world
