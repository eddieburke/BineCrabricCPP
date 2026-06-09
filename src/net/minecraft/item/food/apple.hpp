#pragma once

#include "net/minecraft/item/FoodItem.hpp"

namespace net::minecraft::item {

class AppleItem : public FoodItem {
public:
    static constexpr int ID = 260;
    AppleItem() : FoodItem(4, 4, false) {
        setTexturePosition(10, 0)->setTranslationKey("apple");
    }
};

} // namespace net::minecraft::item
