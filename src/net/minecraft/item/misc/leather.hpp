#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class LeatherItem : public Item {
public:
    static constexpr int ID = 334;
    LeatherItem() : Item(78) {
        setTexturePosition(7, 6)->setTranslationKey("leather");
    }
};

} // namespace net::minecraft::item
