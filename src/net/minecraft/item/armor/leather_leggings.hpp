#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::item {

class LeatherLeggingsItem : public ArmorItem {
public:
    static constexpr int ID = 300;
    LeatherLeggingsItem() : ArmorItem(44, 0, 0, 2) {
        setTexturePosition(0, 2)->setTranslationKey("leggingsCloth");
    }
};

} // namespace net::minecraft::item
