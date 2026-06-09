#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class BrickItem : public Item {
public:
    static constexpr int ID = 336;
    BrickItem() : Item(80) {
        setTexturePosition(6, 1)->setTranslationKey("brick");
    }
};

} // namespace net::minecraft::item
