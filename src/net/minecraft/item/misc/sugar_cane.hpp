#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/SecondaryBlockItem.hpp"

namespace net::minecraft::item {

class SugarCaneItem : public SecondaryBlockItem {
public:
    static constexpr int ID = 338;
    SugarCaneItem() : SecondaryBlockItem(82, Block::SUGAR_CANE) {
        setTexturePosition(11, 1)->setTranslationKey("reeds");
    }
};

} // namespace net::minecraft::item
