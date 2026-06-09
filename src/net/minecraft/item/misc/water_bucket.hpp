#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/BucketItem.hpp"

namespace net::minecraft::item {

class WaterBucketItem : public BucketItem {
public:
    static constexpr int ID = 326;
    WaterBucketItem() : BucketItem(70, Block::FLOWING_WATER != nullptr ? Block::FLOWING_WATER->id : 8) {
        setCraftingReturnItem(Item::ITEMS[325]);
        setTexturePosition(11, 4)->setTranslationKey("bucketWater");
    }
};

} // namespace net::minecraft::item
