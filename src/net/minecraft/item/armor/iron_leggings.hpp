#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::item {

class IronLeggingsItem : public ArmorItem {
public:
    static constexpr int ID = 308;
    IronLeggingsItem() : ArmorItem(52, 2, 2, 2) {
        setTexturePosition(2, 2)->setTranslationKey("leggingsIron");
    }
};

} // namespace net::minecraft::item
