#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class DiamondItem : public Item {
public:
    static constexpr int ID = 264;
    DiamondItem() : Item(8) {
        setTexturePosition(7, 3)->setTranslationKey("emerald");
    }
};

} // namespace net::minecraft::item
