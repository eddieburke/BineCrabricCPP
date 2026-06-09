#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/SecondaryBlockItem.hpp"

namespace net::minecraft::item {

class CakeItem : public SecondaryBlockItem {
public:
    static constexpr int ID = 354;
    CakeItem() : SecondaryBlockItem(98, Block::CAKE) {
        setMaxCount(1);
        setTexturePosition(13, 1)->setTranslationKey("cake");
    }
};

} // namespace net::minecraft::item
