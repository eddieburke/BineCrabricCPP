#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::item {

class DiamondBootsItem : public ArmorItem {
public:
    static constexpr int ID = 313;
    DiamondBootsItem() : ArmorItem(57, 3, 3, 3) {
        setTexturePosition(3, 3)->setTranslationKey("bootsDiamond");
    }
};

} // namespace net::minecraft::item
