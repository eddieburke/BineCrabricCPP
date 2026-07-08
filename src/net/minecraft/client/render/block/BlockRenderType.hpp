#pragma once

namespace net::minecraft::client::render::block {
// Named values for Block::getRenderType(), which returns an int in the faithful
// beta 1.7.3 port. These replace the magic 0-17 literals scattered through the
// render dispatch. Values must match the originals exactly.
namespace BlockRenderType {
inline constexpr int FULL_CUBE = 0;
inline constexpr int CROSS = 1;  // flowers, saplings, mushrooms
inline constexpr int TORCH = 2;
inline constexpr int FIRE = 3;
inline constexpr int FLUID = 4;
inline constexpr int REDSTONE_DUST = 5;
inline constexpr int CROP = 6;
inline constexpr int DOOR = 7;
inline constexpr int LADDER = 8;
inline constexpr int RAIL = 9;
inline constexpr int STAIRS = 10;
inline constexpr int FENCE = 11;
inline constexpr int LEVER = 12;
inline constexpr int CACTUS = 13;
inline constexpr int BED = 14;
inline constexpr int REPEATER = 15;
inline constexpr int PISTON = 16;
inline constexpr int PISTON_HEAD = 17;
}  // namespace BlockRenderType
}  // namespace net::minecraft::client::render::block
