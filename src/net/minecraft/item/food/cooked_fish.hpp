#pragma once

#include "net/minecraft/item/FoodItem.hpp"

namespace net::minecraft::item {

class CookedFishItem : public FoodItem {
public:
    static constexpr int ID = 350;
    CookedFishItem() : FoodItem(94, 5, false) {
        setTexturePosition(10, 5)->setTranslationKey("fishCooked");
    }
};

} // namespace net::minecraft::item
