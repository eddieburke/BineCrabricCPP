#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class StickItem : public Item {
public:
    static constexpr int ID = 280;
    StickItem() : Item(24) {
        setHandheld();
        setTexturePosition(5, 3)->setTranslationKey("stick");
    }
};

} // namespace net::minecraft::item
