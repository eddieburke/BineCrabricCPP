#pragma once

#include "net/minecraft/item/MinecartItem.hpp"

namespace net::minecraft::item {

class ChestMinecartItem : public MinecartItem {
public:
    static constexpr int ID = 342;
    ChestMinecartItem() : MinecartItem(86, 1) {
        setTexturePosition(7, 9)->setTranslationKey("minecartChest");
    }
};

} // namespace net::minecraft::item
