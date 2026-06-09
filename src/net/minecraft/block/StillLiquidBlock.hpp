#pragma once

#include "net/minecraft/block/LiquidBlock.hpp"

namespace net::minecraft::block {

class StillLiquidBlock : public LiquidBlock {
public:
    StillLiquidBlock(int id, Material& mat);

    void neighborUpdate(World* world, int x, int y, int z, int id) override;
    void onTick(World* world, int x, int y, int z, JavaRandom& random) override;

private:
    void convertToFlowing(World* world, int x, int y, int z);
    [[nodiscard]] bool isFlammable(World* world, int x, int y, int z) const;
};

} // namespace net::minecraft::block
