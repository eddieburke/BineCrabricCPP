#pragma once

#include "net/minecraft/item/BoatItem.hpp"

namespace net::minecraft::item::vanilla {

class BoatItem : public item::BoatItem {
public:
    static constexpr int ID = 333;
    BoatItem() : item::BoatItem(77) {
        setTexturePosition(8, 8)->setTranslationKey("boat");
    }
};

} // namespace net::minecraft::item::vanilla
