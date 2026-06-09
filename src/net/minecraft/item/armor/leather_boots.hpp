#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::item {

class LeatherBootsItem : public ArmorItem {
public:
    static constexpr int ID = 301;
    LeatherBootsItem() : ArmorItem(45, 0, 0, 3) {
        setTexturePosition(0, 3)->setTranslationKey("bootsCloth");
    }
};

} // namespace net::minecraft::item
