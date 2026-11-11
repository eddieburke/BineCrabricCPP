#pragma once
#include <array>
#include "net/minecraft/block/LiquidBlock.hpp"
namespace net::minecraft::block {
class FlowingLiquidBlock : public LiquidBlock {
 public:
 static constexpr int kBlockId = 8;
 static void registerClass();
 FlowingLiquidBlock(int id, Material& mat) : LiquidBlock(id, mat) {
 }
 void onTick(World* world, int x, int y, int z, JavaRandom& random) override;
 void onPlaced(World* world, int x, int y, int z) override;
 void neighborUpdate(World* world, int x, int y, int z, int neighborId) override;

 private:
 void convertToSource(World* world, int x, int y, int z);
 void spreadTo(World* world, int x, int y, int z, int depth);
 [[nodiscard]] int getDistanceToGap(World* world, int x, int y, int z, int distance, int fromDirection) const;
 [[nodiscard]] std::array<bool, 4> getSpread(World* world, int x, int y, int z) const;
 [[nodiscard]] bool isLiquidBreaking(int blockId) const;
 [[nodiscard]] bool isLiquidBreaking(World* world, int x, int y, int z) const;
 [[nodiscard]] int getLowestDepth(World* world, int x, int y, int z, int currentLowest, int& sourceCount) const;
 [[nodiscard]] bool canSpreadTo(World* world, int x, int y, int z) const;
};
} // namespace net::minecraft::block
