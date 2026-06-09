#pragma once

#include "net/minecraft/item/FoodItem.hpp"

namespace net::minecraft::item {

class RawFishItem : public FoodItem {
public:
    static constexpr int ID = 349;
    RawFishItem() : FoodItem(93, 2, false) {
        setTexturePosition(9, 5)->setTranslationKey("fishRaw");
    }
};

} // namespace net::minecraft::item
