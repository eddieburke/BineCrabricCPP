#pragma once

#include <array>

namespace net::minecraft::block {

class MapColor {
public:
    inline static std::array<const MapColor*, 16> COLORS {};

    static const MapColor CLEAR;
    static const MapColor PALE_GREEN;
    static const MapColor PALE_YELLOW;
    static const MapColor LIGHT_GRAY;
    static const MapColor RED;
    static const MapColor LIGHT_BLUE;
    static const MapColor LIGHT_GRAY2;
    static const MapColor GREEN;
    static const MapColor WHITE;
    static const MapColor SILVER;
    static const MapColor ORANGE;
    static const MapColor GRAY;
    static const MapColor BLUE;
    static const MapColor BROWN;

    const int id;
    const int color;

    constexpr MapColor(int id, int color)
        : id(id),
          color(color)
    {
    }
};

inline const MapColor MapColor::CLEAR {0, 0};
inline const MapColor MapColor::PALE_GREEN {1, 8368696};
inline const MapColor MapColor::PALE_YELLOW {2, 16247203};
inline const MapColor MapColor::LIGHT_GRAY {3, 0xA7A7A7};
inline const MapColor MapColor::RED {4, 0xFF0000};
inline const MapColor MapColor::LIGHT_BLUE {5, 0xA0A0FF};
inline const MapColor MapColor::LIGHT_GRAY2 {6, 0xA7A7A7};
inline const MapColor MapColor::GREEN {7, 31744};
inline const MapColor MapColor::WHITE {8, 0xFFFFFF};
inline const MapColor MapColor::SILVER {9, 10791096};
inline const MapColor MapColor::ORANGE {10, 12020271};
inline const MapColor MapColor::GRAY {11, 0x707070};
inline const MapColor MapColor::BLUE {12, 0x4040FF};
inline const MapColor MapColor::BROWN {13, 6837042};

inline void initializeMapColors()
{
    MapColor::COLORS[0] = &MapColor::CLEAR;
    MapColor::COLORS[1] = &MapColor::PALE_GREEN;
    MapColor::COLORS[2] = &MapColor::PALE_YELLOW;
    MapColor::COLORS[3] = &MapColor::LIGHT_GRAY;
    MapColor::COLORS[4] = &MapColor::RED;
    MapColor::COLORS[5] = &MapColor::LIGHT_BLUE;
    MapColor::COLORS[6] = &MapColor::LIGHT_GRAY2;
    MapColor::COLORS[7] = &MapColor::GREEN;
    MapColor::COLORS[8] = &MapColor::WHITE;
    MapColor::COLORS[9] = &MapColor::SILVER;
    MapColor::COLORS[10] = &MapColor::ORANGE;
    MapColor::COLORS[11] = &MapColor::GRAY;
    MapColor::COLORS[12] = &MapColor::BLUE;
    MapColor::COLORS[13] = &MapColor::BROWN;
}

} // namespace net::minecraft::block
