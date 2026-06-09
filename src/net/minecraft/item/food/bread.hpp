#pragma once

#include "net/minecraft/item/FoodItem.hpp"

namespace net::minecraft::item {

class BreadItem : public FoodItem {
public:
    static constexpr int ID = 297;
    BreadItem() : FoodItem(41, 5, false) {
        setTexturePosition(9, 2)->setTranslationKey("bread");
    }
};

} // namespace net::minecraft::item
