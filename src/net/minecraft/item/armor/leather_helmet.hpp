#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::item {

class LeatherHelmetItem : public ArmorItem {
public:
    static constexpr int ID = 298;
    LeatherHelmetItem() : ArmorItem(42, 0, 0, 0) {
        setTexturePosition(0, 0)->setTranslationKey("helmetCloth");
    }
};

} // namespace net::minecraft::item
