#pragma once
#include "net/minecraft/item/MinecartItem.hpp"

namespace net::minecraft::item::vanilla {
class MinecartItem : public item::MinecartItem {
   public:
    static constexpr int ID = 328;

    MinecartItem() : item::MinecartItem(72, 0) {
        setTexturePosition(7, 8)->setTranslationKey("minecart");
    }
};
}  // namespace net::minecraft::item::vanilla
