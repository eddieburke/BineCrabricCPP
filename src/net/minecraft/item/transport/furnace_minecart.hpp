#pragma once

#include "net/minecraft/item/MinecartItem.hpp"

namespace net::minecraft::item {

class FurnaceMinecartItem : public MinecartItem {
public:
    static constexpr int ID = 343;
    FurnaceMinecartItem() : MinecartItem(87, 2) {
        setTexturePosition(7, 10)->setTranslationKey("minecartFurnace");
    }
};

} // namespace net::minecraft::item
