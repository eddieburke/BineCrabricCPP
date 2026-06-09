#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::item {

class ChainBootsItem : public ArmorItem {
public:
    static constexpr int ID = 305;
    ChainBootsItem() : ArmorItem(49, 1, 1, 3) {
        setTexturePosition(1, 3)->setTranslationKey("bootsChain");
    }
};

} // namespace net::minecraft::item
