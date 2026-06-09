#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/SecondaryBlockItem.hpp"

namespace net::minecraft::item {

class RepeaterItem : public SecondaryBlockItem {
public:
    static constexpr int ID = 356;
    RepeaterItem() : SecondaryBlockItem(100, Block::REPEATER) {
        setTexturePosition(6, 5)->setTranslationKey("diode");
    }
};

} // namespace net::minecraft::item
