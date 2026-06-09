#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::item {

class DiamondLeggingsItem : public ArmorItem {
public:
    static constexpr int ID = 312;
    DiamondLeggingsItem() : ArmorItem(56, 3, 3, 2) {
        setTexturePosition(3, 2)->setTranslationKey("leggingsDiamond");
    }
};

} // namespace net::minecraft::item
