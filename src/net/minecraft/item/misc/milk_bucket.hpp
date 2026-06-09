#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/BucketItem.hpp"

namespace net::minecraft::item {

class MilkBucketItem : public BucketItem {
public:
    static constexpr int ID = 335;
    MilkBucketItem() : BucketItem(79, -1) {
        setCraftingReturnItem(Item::ITEMS[325]);
        setTexturePosition(13, 4)->setTranslationKey("milk");
    }
};

} // namespace net::minecraft::item
