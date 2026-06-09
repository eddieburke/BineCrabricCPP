#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class IronIngotItem : public Item {
public:
    static constexpr int ID = 265;
    IronIngotItem() : Item(9) {
        setTexturePosition(7, 1)->setTranslationKey("ingotIron");
    }
};

} // namespace net::minecraft::item
