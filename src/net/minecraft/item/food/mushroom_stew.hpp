#pragma once
#include "net/minecraft/item/MushroomStewItem.hpp"

namespace net::minecraft::item::vanilla {
class MushroomStewItem : public item::MushroomStewItem {
   public:
    static constexpr int ID = 282;

    MushroomStewItem() : item::MushroomStewItem(26, 10) {
        setTexturePosition(8, 4)->setTranslationKey("mushroomStew");
    }
};
}  // namespace net::minecraft::item::vanilla
