#pragma once
#include <array>

namespace net::minecraft::block {
struct PistonConstants {
    inline static constexpr int TEXTURE_STICKY_TOP = 106;
    inline static constexpr int TEXTURE_TOP = 107;
    inline static constexpr int TEXTURE_EXTENSION = 108;
    inline static constexpr int TEXTURE_INSIDE = 109;
    inline static constexpr int TEXTURE_FACE_EXTENDED = 110;
    inline static constexpr std::array<int, 6> TEXTURE_SIDES{1, 0, 3, 2, 5, 4};
    inline static constexpr std::array<int, 6> HEAD_OFFSET_X{0, 0, 0, 0, -1, 1};
    inline static constexpr std::array<int, 6> HEAD_OFFSET_Y{-1, 1, 0, 0, 0, 0};
    inline static constexpr std::array<int, 6> HEAD_OFFSET_Z{0, 0, -1, 1, 0, 0};
};
}  // namespace net::minecraft::block
