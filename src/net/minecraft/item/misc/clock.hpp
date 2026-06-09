#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class ClockItem : public Item {
public:
    static constexpr int ID = 347;
    ClockItem() : Item(91) {
        setTexturePosition(6, 4)->setTranslationKey("clock");
    }
};

} // namespace net::minecraft::item
