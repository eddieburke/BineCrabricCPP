#pragma once

#include <cstddef>
#include <cstdint>

namespace net::minecraft::client::render::world {

// Position of a 16^3 render section in section units: x/z are chunkX/chunkZ,
// y is the vertical section index (0..7 for a 128-tall world).
struct SectionPos {
    int x = 0;
    int y = 0;
    int z = 0;

    [[nodiscard]] bool operator==(const SectionPos& other) const noexcept
    {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct SectionPosHash {
    [[nodiscard]] std::size_t operator()(const SectionPos& p) const noexcept
    {
        std::size_t h = static_cast<std::uint32_t>(p.x) * 0x9E3779B1u;
        h ^= static_cast<std::uint32_t>(p.z) * 0x85EBCA77u + (h << 6) + (h >> 2);
        h ^= static_cast<std::uint32_t>(p.y) * 0xC2B2AE3Du + (h << 6) + (h >> 2);
        return h;
    }
};

} // namespace net::minecraft::client::render::world
