#pragma once

#include "net/minecraft/item/BlockItem.hpp"

namespace net::minecraft::item {

class LogBlockItem : public BlockItem {
public:
    explicit LogBlockItem(int rawId) : BlockItem(rawId)
    {
        setMaxDamage(0);
        setHasSubtypes(true);
    }

    [[nodiscard]] int getPlacementMetadata(int meta) const override { return meta; }
    [[nodiscard]] int getTextureId(int damage) const override
    {
        return Block::LOG != nullptr ? Block::LOG->getTexture(2, damage) : BlockItem::getTextureId(damage);
    }
};

} // namespace net::minecraft::item
