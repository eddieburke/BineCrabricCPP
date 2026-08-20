#pragma once
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::block {
// Beta's horizontal neighbour order for the 4-way scans (fluid spread, redstone wire
// power search): 0 = -X, 1 = +X, 2 = -Z, 3 = +Z. FlowingLiquidBlock and
// RedstoneWireBlock each open-coded this if/else chain twice.
inline constexpr int kHorizontalOffsetX[4] = {-1, 1, 0, 0};
inline constexpr int kHorizontalOffsetZ[4] = {0, 0, -1, 1};
// Yaw quadrant (floor(yaw * 4 / 360 + 0.5) & 3) to the block FACE a placed block
// takes: 2 = north, 5 = east, 3 = south, 4 = west.
// see third_party/mcp/net/minecraft/src/BlockFurnace.java:123
// see third_party/mcp/net/minecraft/src/BlockPistonBase.java:299
// StairsBlock deliberately does NOT use this: Java gives stairs {2, 1, 3, 0}, which
// is a stair orientation rather than a face, and the two tables only look alike.
// see third_party/mcp/net/minecraft/src/BlockStairs.java:180
inline constexpr int kYawQuadrantFacing[4] = {2, 5, 3, 4};
// A wall attachment (button, lever, ladder, torch) hangs on any of the four
// horizontal neighbours being a suffocating block. ButtonBlock, LeverBlock,
// LadderBlock and TorchBlock each carried their own copy of this chain.
[[nodiscard]] inline bool anyHorizontalNeighborSuffocates(net::minecraft::World* world, int x, int y, int z) {
 return world->shouldSuffocate(x - 1, y, z) || world->shouldSuffocate(x + 1, y, z) ||
        world->shouldSuffocate(x, y, z - 1) || world->shouldSuffocate(x, y, z + 1);
}
// The block a face-attached placement would hang on, by side index: 1 is the
// floor below, 2..5 are the four walls. Anything else has nothing behind it.
[[nodiscard]] inline bool sideSuffocates(net::minecraft::World* world, int x, int y, int z, int side) {
 switch(side) {
 case 1:
  return world->shouldSuffocate(x, y - 1, z);
 case 2:
  return world->shouldSuffocate(x, y, z + 1);
 case 3:
  return world->shouldSuffocate(x, y, z - 1);
 case 4:
  return world->shouldSuffocate(x + 1, y, z);
 case 5:
  return world->shouldSuffocate(x - 1, y, z);
 default:
  return false;
 }
}
} // namespace net::minecraft::block
