#pragma once

#include "net/minecraft/item/BucketItem.hpp"

namespace net::minecraft::item::vanilla {

class BucketItem : public item::BucketItem {
public:
    static constexpr int ID = 325;
    BucketItem() : item::BucketItem(69, 0) {
        setTexturePosition(10, 4)->setTranslationKey("bucket");
    }
};

} // namespace net::minecraft::item::vanilla
