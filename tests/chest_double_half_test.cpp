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
// Single chest faces: 26 on the plain sides, 27 on the front.
constexpr int kSmallSide = 26;
constexpr int kSmallFront = 27;
// Double chest backs, in the same left/right order as kLargeFront*.
constexpr int kLargeBackLeft = 57;
constexpr int kLargeBackRight = 58;
class FakeBlockView : public BlockView {
 public:
 void set(int x, int y, int z, int blockId) {
  blocks_[{x, y, z}] = blockId;
 }
 void setMeta(int x, int y, int z, int meta) {
  metas_[{x, y, z}] = meta;
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
 [[nodiscard]] int getBlockMeta(int x, int y, int z) const override {
  const auto found = metas_.find({x, y, z});
  return found == metas_.end() ? 0 : found->second;
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
 std::map<std::tuple<int, int, int>, int> metas_;
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
// A chest placed by a player stores the side its front faces; nothing in the surroundings
// gets a vote once that is set.
TEST(ChestFacing, StoredFacingPutsTheFrontOnThatSide) {
 FakeBlockView view;
 view.set(10, kY, 5, Block::CHEST->id);
 view.setMeta(10, kY, 5, kSideWest);
 EXPECT_EQ(Block::CHEST->getTextureId(&view, 10, kY, 5, kSideWest), kSmallFront);
 EXPECT_EQ(Block::CHEST->getTextureId(&view, 10, kY, 5, kSideEast), kSmallSide);
 EXPECT_EQ(Block::CHEST->getTextureId(&view, 10, kY, 5, kSideNorth), kSmallSide);
 EXPECT_EQ(Block::CHEST->getTextureId(&view, 10, kY, 5, kSideSouth), kSmallSide);
}
// Beta turns a walled-in chest's front away from the wall. A stored facing outranks that,
// so a chest against a north wall can still open toward the north.
TEST(ChestFacing, StoredFacingOutranksTheWallGuess) {
 FakeBlockView view;
 view.set(10, kY, 5, Block::CHEST->id);
 view.set(10, kY, 4, Block::STONE->id);
 EXPECT_EQ(Block::CHEST->getTextureId(&view, 10, kY, 5, kSideSouth), kSmallFront);
 view.setMeta(10, kY, 5, kSideNorth);
 EXPECT_EQ(Block::CHEST->getTextureId(&view, 10, kY, 5, kSideNorth), kSmallFront);
 EXPECT_EQ(Block::CHEST->getTextureId(&view, 10, kY, 5, kSideSouth), kSmallSide);
}
// Only one half needs the facing: an unset partner reads it off the half that has it, so a
// world-gen chest joined to a placed one cannot end up front-to-back with it.
TEST(ChestFacing, DoubleChestReadsTheFacingOffEitherHalf) {
 FakeBlockView view;
 view.set(10, kY, 5, Block::CHEST->id);
 view.set(10, kY, 6, Block::CHEST->id);
 view.setMeta(10, kY, 5, kSideEast);
 // Standing east looking west puts +Z on the viewer's left, so (10, 6) is the left half.
 EXPECT_EQ(Block::CHEST->getTextureId(&view, 10, kY, 6, kSideEast), kLargeFrontLeft);
 EXPECT_EQ(Block::CHEST->getTextureId(&view, 10, kY, 5, kSideEast), kLargeFrontRight);
 // The back is seen from the other side, so the halves swap over.
 EXPECT_EQ(Block::CHEST->getTextureId(&view, 10, kY, 5, kSideWest), kLargeBackLeft);
 EXPECT_EQ(Block::CHEST->getTextureId(&view, 10, kY, 6, kSideWest), kLargeBackRight);
}
// A double chest's front sits on its long side, so a facing pointing down the run belongs to
// neither half. Rather than render a chest with no front at all, fall back to the wall guess.
TEST(ChestFacing, DoubleChestIgnoresAFacingDownTheRun) {
 FakeBlockView view;
 view.set(10, kY, 5, Block::CHEST->id);
 view.set(10, kY, 6, Block::CHEST->id);
 view.setMeta(10, kY, 5, kSideNorth);
 view.setMeta(10, kY, 6, kSideNorth);
 view.set(9, kY, 5, Block::STONE->id);
 view.set(9, kY, 6, Block::STONE->id);
 EXPECT_EQ(Block::CHEST->getTextureId(&view, 10, kY, 6, kSideEast), kLargeFrontLeft);
 EXPECT_EQ(Block::CHEST->getTextureId(&view, 10, kY, 5, kSideEast), kLargeFrontRight);
}
