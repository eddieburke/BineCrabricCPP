#pragma once
#include "net/minecraft/item/BlockItem.hpp"

namespace net::minecraft::item {
class LeavesBlockItem : public BlockItem {
   public:
    explicit LeavesBlockItem(int rawId) : BlockItem(rawId) {
        setMaxDamage(0);
        setHasSubtypes(true);
    }

    [[nodiscard]] int getPlacementMetadata(int meta) const override {
        return meta | 8;
    }

    [[nodiscard]] int getTextureId(int damage) const override {
        return Block::LEAVES != nullptr ? Block::LEAVES->getTexture(0, damage) : BlockItem::getTextureId(damage);
    }

    [[nodiscard]] int getColorMultiplier(int color) const override {
        if ((color & 1) == 1) {
            return 0x619961;
        }
        if ((color & 2) == 2) {
            return 0x80A755;
        }
        return 0x48B518;
    }
};
}  // namespace net::minecraft::item
