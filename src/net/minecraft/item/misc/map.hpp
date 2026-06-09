#pragma once

#include "net/minecraft/item/MapItem.hpp"

namespace net::minecraft::item::vanilla {

class MapItem : public item::MapItem {
public:
    static constexpr int ID = 358;
    MapItem() : item::MapItem(102) {
        setTexturePosition(12, 3)->setTranslationKey("map");
    }
};

} // namespace net::minecraft::item::vanilla
