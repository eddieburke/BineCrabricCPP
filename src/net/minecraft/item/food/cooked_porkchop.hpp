#pragma once

#include "net/minecraft/item/FoodItem.hpp"

namespace net::minecraft::item {

class CookedPorkchopItem : public FoodItem {
public:
    static constexpr int ID = 320;
    CookedPorkchopItem() : FoodItem(64, 8, true) {
        setTexturePosition(8, 5)->setTranslationKey("porkchopCooked");
    }
};

} // namespace net::minecraft::item
