#include <cstddef>
#include <gtest/gtest.h>
#include <map>
#include <tuple>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/world/BlockView.hpp"
namespace {
using net::minecraft::BlockView;
using net::minecraft::block::Block;
// terrain.png: 41 carries the latch on its right edge, 42 on its left edge, so a double
// chest only closes over its latch when 41 is the half on the viewer's left.
constexpr int kLargeFrontLeft = 41;
constexpr int kLargeFrontRight = 42;
// Beta side ids: 2 = -Z, 3 = +Z, 4 = -X, 5 = +X.
constexpr int kSideNorth = 2;
constexpr int kSideSouth = 3;
constexpr int kSideWest = 4;
constexpr int kSideEast = 5;
constexpr int kY = 64;
class FakeBlockView : public BlockView {
 public:
 void set(int x, int y, int z, int blockId) {
  blocks_[{x, y, z}] = blockId;
 }
 [[nodiscard]] int getBlockId(int x, int y, int z) const override {
  const auto found = blocks_.find({x, y, z});
  return found == blocks_.end() ? 0 : found->second;
 }
 [[nodiscard]] net::minecraft::block::entity::BlockEntity* getBlockEntity(int, int, int) override {
  return nullptr;
 }
 [[nodiscard]] float getNaturalBrightness(int, int, int, int) const override {
  return 1.0f;
 }
 [[nodiscard]] float getLightBrightness(int, int, int) const override {
  return 1.0f;
 }
 [[nodiscard]] int getBlockMeta(int, int, int) const override {
  return 0;
 }
 [[nodiscard]] net::minecraft::block::material::Material& getMaterial(int, int, int) const override {
  return net::minecraft::block::material::Material::AIR;
 }
 [[nodiscard]] bool isBlockOpaqueCube(int x, int y, int z) const override {
  return Block::BLOCKS_OPAQUE[static_cast<std::size_t>(getBlockId(x, y, z))];
 }
 [[nodiscard]] bool shouldSuffocate(int x, int y, int z) const override {
  return isBlockOpaqueCube(x, y, z);
 }
 [[nodiscard]] net::minecraft::BiomeSource* getBiomeSource() const override {
  return nullptr;
 }

 private:
 std::map<std::tuple<int, int, int>, int> blocks_;
};
// Lays a two-block chest run plus the wall that decides which way it faces, then reports
// the front texture of each half in the order a player standing at the front sees them,
// left to right.
struct DoubleChest {
 int leftTexture;
 int rightTexture;
};
DoubleChest frontHalvesLeftToRight(int aX, int aZ, int bX, int bZ, int wallDx, int wallDz, int frontSide) {
 FakeBlockView view;
 const int chest = Block::CHEST->id;
 const int stone = Block::STONE->id;
 view.set(aX, kY, aZ, chest);
 view.set(bX, kY, bZ, chest);
 view.set(aX + wallDx, kY, aZ + wallDz, stone);
 view.set(bX + wallDx, kY, bZ + wallDz, stone);
 // a is the half on the viewer's left by construction of each case below.
 return {Block::CHEST->getTextureId(&view, aX, kY, aZ, frontSide),
         Block::CHEST->getTextureId(&view, bX, kY, bZ, frontSide)};
}
} // namespace
// A player facing -Z sees +X on their right, so the western block is the left half.
TEST(ChestDoubleHalf, RunAlongXFacingSouthPutsWesternBlockOnTheLeft) {
 const DoubleChest halves = frontHalvesLeftToRight(10, 5, 11, 5, 0, -1, kSideSouth);
 EXPECT_EQ(halves.leftTexture, kLargeFrontLeft);
 EXPECT_EQ(halves.rightTexture, kLargeFrontRight);
}
// Facing +Z reverses it: the eastern block is now the one on the viewer's left.
TEST(ChestDoubleHalf, RunAlongXFacingNorthPutsEasternBlockOnTheLeft) {
 const DoubleChest halves = frontHalvesLeftToRight(11, 5, 10, 5, 0, 1, kSideNorth);
 EXPECT_EQ(halves.leftTexture, kLargeFrontLeft);
 EXPECT_EQ(halves.rightTexture, kLargeFrontRight);
}
// A player facing +X sees -Z on their left, so the northern block is the left half.
TEST(ChestDoubleHalf, RunAlongZFacingWestPutsNorthernBlockOnTheLeft) {
 const DoubleChest halves = frontHalvesLeftToRight(5, 10, 5, 11, 1, 0, kSideWest);
 EXPECT_EQ(halves.leftTexture, kLargeFrontLeft);
 EXPECT_EQ(halves.rightTexture, kLargeFrontRight);
}
// Facing -X reverses it: the southern block is the one on the viewer's left.
TEST(ChestDoubleHalf, RunAlongZFacingEastPutsSouthernBlockOnTheLeft) {
 const DoubleChest halves = frontHalvesLeftToRight(5, 11, 5, 10, -1, 0, kSideEast);
 EXPECT_EQ(halves.leftTexture, kLargeFrontLeft);
 EXPECT_EQ(halves.rightTexture, kLargeFrontRight);
}
