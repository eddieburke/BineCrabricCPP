#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class FlintItem : public Item {
public:
    static constexpr int ID = 318;
    FlintItem() : Item(62) {
        setTexturePosition(6, 0)->setTranslationKey("flint");
    }
};

} // namespace net::minecraft::item
