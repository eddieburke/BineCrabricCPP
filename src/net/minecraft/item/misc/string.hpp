#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class StringItem : public Item {
public:
    static constexpr int ID = 287;
    StringItem() : Item(31) {
        setTexturePosition(8, 0)->setTranslationKey("string");
    }
};

} // namespace net::minecraft::item
