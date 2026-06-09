#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class ClayItem : public Item {
public:
    static constexpr int ID = 337;
    ClayItem() : Item(81) {
        setTexturePosition(9, 3)->setTranslationKey("clay");
    }
};

} // namespace net::minecraft::item
