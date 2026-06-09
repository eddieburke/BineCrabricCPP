#pragma once

#include <array>

namespace net::minecraft::block {

struct PistonConstants {
    inline static constexpr std::array<int, 6> TEXTURE_SIDES {1, 0, 3, 2, 5, 4};
    inline static constexpr std::array<int, 6> HEAD_OFFSET_X {0, 0, 0, 0, -1, 1};
    inline static constexpr std::array<int, 6> HEAD_OFFSET_Y {-1, 1, 0, 0, 0, 0};
    inline static constexpr std::array<int, 6> HEAD_OFFSET_Z {0, 0, -1, 1, 0, 0};
};

} // namespace net::minecraft::block
