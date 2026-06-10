#pragma once

#include "net/minecraft/item/FoodItem.hpp"

namespace net::minecraft::item {

class StackableFoodItem : public FoodItem {
public:
    static void registerClass();
    StackableFoodItem(int rawId, int healthRestored, bool meat, int maxCount)
        : FoodItem(rawId, healthRestored, meat)
    {
        setMaxCount(maxCount);
    }
};

} // namespace net::minecraft::item
