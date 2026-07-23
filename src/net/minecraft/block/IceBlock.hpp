#pragma once
#include "net/minecraft/block/TranslucentBlock.hpp"
namespace net::minecraft::block {
class IceBlock : public TranslucentBlock {
 public:
 static constexpr int kBlockId = 79;
 static void registerClass();
 IceBlock(int id, int textureId);
 [[nodiscard]] int getRenderLayer() const override {
  return 1;
 }
 [[nodiscard]] int getDroppedItemCount(JavaRandom& /*random*/) const override {
  return 0;
 }
 [[nodiscard]] int getPistonBehavior() const override {
  return 0;
 }
 [[nodiscard]] bool isSideVisibleForBounds(
     const BlockView* blockView, int x, int y, int z, int side, const net::minecraft::Box& bounds) const override;
 void afterBreak(World* world, net::minecraft::PlayerEntity* player, int x, int y, int z, int meta) override;
 void onTick(World* world, int x, int y, int z, JavaRandom& random) override;
};
} // namespace net::minecraft::block
