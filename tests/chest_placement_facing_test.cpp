#include <gtest/gtest.h>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/multiplayer/MultiplayerClientPlayerEntity.hpp"
#include "net/minecraft/client/util/Session.hpp"
#include "net/minecraft/world/ClientWorld.hpp"
namespace net::minecraft::test {
namespace {
using net::minecraft::block::Block;
// Beta side ids: 2 = -Z, 3 = +Z, 4 = -X, 5 = +X. A chest stores the side its front faces.
constexpr int kFrontNegZ = 2;
constexpr int kFrontPosZ = 3;
constexpr int kFrontNegX = 4;
constexpr int kFrontPosX = 5;
constexpr int kY = 64;
// Beta yaw: 0 looks toward +Z, 90 toward -X, 180 toward -Z, 270 toward +X.
constexpr float kYawLookingPosZ = 0.0f;
constexpr float kYawLookingNegX = 90.0f;
constexpr float kYawLookingNegZ = 180.0f;
constexpr float kYawLookingPosX = 270.0f;
struct ChestPlacement {
 client::multiplayer::ClientNetworkHandler handler{nullptr};
 ClientWorld world{&handler, 12345ULL, 0};
 client::multiplayer::MultiplayerClientPlayerEntity player{nullptr, &world, client::util::Session{}, &handler};
 ChestPlacement() {
  world.updateChunk(0, 0, true);
 }
 // Stands the player where they would be to look at (x, kY, z) from the given yaw, then runs
 // the placement hook the block item calls.
 void place(int x, int z, float yaw, double playerX, double playerZ) {
  player.yaw = yaw;
  player.setPosition(playerX, static_cast<double>(kY), playerZ);
  world.setBlock(x, kY, z, Block::CHEST->id);
  Block::CHEST->onPlaced(&world, x, kY, z, &player);
 }
 [[nodiscard]] int front(int x, int z) const {
  return world.getBlockMeta(x, kY, z);
 }
};
} // namespace
// The whole complaint: the front turns to meet whoever put the chest down.
TEST(ChestPlacementFacing, FrontTurnsTowardThePlacer) {
 ChestPlacement placement;
 placement.place(8, 8, kYawLookingPosZ, 8.5, 6.0);
 EXPECT_EQ(placement.front(8, 8), kFrontNegZ);
 placement.place(8, 10, kYawLookingNegX, 10.0, 10.5);
 EXPECT_EQ(placement.front(8, 10), kFrontPosX);
 placement.place(10, 8, kYawLookingNegZ, 10.5, 10.0);
 EXPECT_EQ(placement.front(10, 8), kFrontPosZ);
 placement.place(10, 10, kYawLookingPosX, 8.0, 10.5);
 EXPECT_EQ(placement.front(10, 10), kFrontNegX);
}
// The second half joins the chest already there rather than spinning the pair around.
TEST(ChestPlacementFacing, SecondHalfAdoptsThePartnersFacing) {
 ChestPlacement placement;
 placement.place(8, 8, kYawLookingNegX, 10.0, 8.5);
 ASSERT_EQ(placement.front(8, 8), kFrontPosX);
 // Looking +Z alone would ask for a -Z front, which a north/south run cannot carry.
 placement.place(8, 9, kYawLookingPosZ, 8.5, 7.0);
 EXPECT_EQ(placement.front(8, 9), kFrontPosX);
 EXPECT_EQ(placement.front(8, 8), kFrontPosX);
}
// Neither the placer nor the partner offers a facing the run can carry, so the pair turns to
// the long side the placer is standing on.
TEST(ChestPlacementFacing, PairFallsBackToTheSideThePlacerStandsOn) {
 ChestPlacement placement;
 placement.place(8, 8, kYawLookingPosZ, 8.5, 6.0);
 ASSERT_EQ(placement.front(8, 8), kFrontNegZ);
 placement.place(8, 9, kYawLookingPosZ, 6.0, 8.5);
 EXPECT_EQ(placement.front(8, 9), kFrontNegX);
 EXPECT_EQ(placement.front(8, 8), kFrontNegX);
 ChestPlacement fromTheEast;
 fromTheEast.place(8, 8, kYawLookingPosZ, 8.5, 6.0);
 fromTheEast.place(8, 9, kYawLookingPosZ, 11.0, 8.5);
 EXPECT_EQ(fromTheEast.front(8, 9), kFrontPosX);
 EXPECT_EQ(fromTheEast.front(8, 8), kFrontPosX);
}
// World generation leaves meta 0 behind, and Beta's wall guess still owns that case.
TEST(ChestPlacementFacing, PlacementWithoutAPlacerLeavesTheFacingUnset) {
 ChestPlacement placement;
 placement.world.setBlock(8, kY, 8, Block::CHEST->id);
 Block::CHEST->onPlaced(&placement.world, 8, kY, 8, static_cast<entity::player::PlayerEntity*>(nullptr));
 EXPECT_EQ(placement.front(8, 8), 0);
}
} // namespace net::minecraft::test
