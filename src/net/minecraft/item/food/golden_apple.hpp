#pragma once

#include "net/minecraft/item/FoodItem.hpp"

namespace net::minecraft::item {

class GoldenAppleItem : public FoodItem {
public:
    static constexpr int ID = 322;
    GoldenAppleItem() : FoodItem(66, 42, false) {
        setTexturePosition(11, 0)->setTranslationKey("appleGold");
    }
};

} // namespace net::minecraft::item
