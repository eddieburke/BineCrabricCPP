#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::item {

class DiamondChestplateItem : public ArmorItem {
public:
    static constexpr int ID = 311;
    DiamondChestplateItem() : ArmorItem(55, 3, 3, 1) {
        setTexturePosition(3, 1)->setTranslationKey("chestplateDiamond");
    }
};

} // namespace net::minecraft::item
