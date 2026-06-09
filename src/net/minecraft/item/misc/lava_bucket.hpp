#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/BucketItem.hpp"

namespace net::minecraft::item {

class LavaBucketItem : public BucketItem {
public:
    static constexpr int ID = 327;
    LavaBucketItem() : BucketItem(71, Block::FLOWING_LAVA != nullptr ? Block::FLOWING_LAVA->id : 10) {
        setCraftingReturnItem(Item::ITEMS[325]);
        setTexturePosition(12, 4)->setTranslationKey("bucketLava");
    }
};

} // namespace net::minecraft::item
