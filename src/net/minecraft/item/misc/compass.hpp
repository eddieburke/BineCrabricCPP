#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class CompassItem : public Item {
public:
    static constexpr int ID = 345;
    CompassItem() : Item(89) {
        setTexturePosition(6, 3)->setTranslationKey("compass");
    }
};

} // namespace net::minecraft::item
