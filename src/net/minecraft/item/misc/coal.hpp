#pragma once

#include "net/minecraft/item/CoalItem.hpp"

namespace net::minecraft::item::vanilla {

class CoalItem : public item::CoalItem {
public:
    static constexpr int ID = 263;
    CoalItem() : item::CoalItem(7) {
        setTexturePosition(7, 0)->setTranslationKey("coal");
    }
};

} // namespace net::minecraft::item::vanilla
