#pragma once

#include "net/minecraft/item/FoodItem.hpp"

namespace net::minecraft::item {

class RawPorkchopItem : public FoodItem {
public:
    static constexpr int ID = 319;
    RawPorkchopItem() : FoodItem(63, 3, true) {
        setTexturePosition(7, 5)->setTranslationKey("porkchopRaw");
    }
};

} // namespace net::minecraft::item
