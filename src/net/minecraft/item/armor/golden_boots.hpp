#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::item {

class GoldenBootsItem : public ArmorItem {
public:
    static constexpr int ID = 317;
    GoldenBootsItem() : ArmorItem(61, 1, 4, 3) {
        setTexturePosition(4, 3)->setTranslationKey("bootsGold");
    }
};

} // namespace net::minecraft::item
