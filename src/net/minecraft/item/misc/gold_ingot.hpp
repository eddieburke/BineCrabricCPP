#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class GoldIngotItem : public Item {
public:
    static constexpr int ID = 266;
    GoldIngotItem() : Item(10) {
        setTexturePosition(7, 2)->setTranslationKey("ingotGold");
    }
};

} // namespace net::minecraft::item
