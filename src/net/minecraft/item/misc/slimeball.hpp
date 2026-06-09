#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class SlimeballItem : public Item {
public:
    static constexpr int ID = 341;
    SlimeballItem() : Item(85) {
        setTexturePosition(14, 1)->setTranslationKey("slimeball");
    }
};

} // namespace net::minecraft::item
