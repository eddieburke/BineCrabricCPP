#pragma once

#include "net/minecraft/block/StoneBlock.hpp"

namespace net::minecraft::block {

class ObsidianBlock : public StoneBlock {
public:
    ObsidianBlock(int id, int textureId) : StoneBlock(id, textureId) {}
    [[nodiscard]] int getDroppedItemId(int /*blockMeta*/, JavaRandom& /*random*/) const override { return 49; }
    [[nodiscard]] int getDroppedItemCount(JavaRandom& /*random*/) const override { return 1; }
};

} // namespace net::minecraft::block