#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class ArrowItem : public Item {
public:
    static constexpr int ID = 262;
    ArrowItem() : Item(6) {
        setTexturePosition(5, 2)->setTranslationKey("arrow");
    }
};

} // namespace net::minecraft::item
