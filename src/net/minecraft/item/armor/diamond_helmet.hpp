#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::item {

class DiamondHelmetItem : public ArmorItem {
public:
    static constexpr int ID = 310;
    DiamondHelmetItem() : ArmorItem(54, 3, 3, 0) {
        setTexturePosition(3, 0)->setTranslationKey("helmetDiamond");
    }
};

} // namespace net::minecraft::item
