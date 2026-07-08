#pragma once
#include "net/minecraft/item/SaddleItem.hpp"

namespace net::minecraft::item::vanilla {
class SaddleItem : public item::SaddleItem {
   public:
    static constexpr int ID = 329;

    SaddleItem() : item::SaddleItem(73) {
        setTexturePosition(8, 6)->setTranslationKey("saddle");
    }
};
}  // namespace net::minecraft::item::vanilla
