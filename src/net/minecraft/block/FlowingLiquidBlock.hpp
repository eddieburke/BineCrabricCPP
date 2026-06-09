#pragma once

#include "net/minecraft/block/LiquidBlock.hpp"

namespace net::minecraft::block {

class FlowingLiquidBlock : public LiquidBlock {
public:
    int adjacentSources = 0;
    bool spread[4] = {};
    int distanceToGap[4] = {};

    FlowingLiquidBlock(int id, Material& mat) : LiquidBlock(id, mat) {}

    void onTick(World* world, int x, int y, int z, JavaRandom& random) override;
    void onPlaced(World* world, int x, int y, int z) override;

private:
    void convertToSource(World* world, int x, int y, int z);
    void spreadTo(World* world, int x, int y, int z, int depth);
    int getDistanceToGap(World* world, int x, int y, int z, int distance, int fromDirection);
    bool* getSpread(World* world, int x, int y, int z);
    [[nodiscard]] bool isLiquidBreaking(World* world, int x, int y, int z) const;
    int getLowestDepth(World* world, int x, int y, int z, int depth);
    [[nodiscard]] bool canSpreadTo(World* world, int x, int y, int z) const;
};

} // namespace net::minecraft::block
