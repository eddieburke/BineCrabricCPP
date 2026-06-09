#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::item {

class IronBootsItem : public ArmorItem {
public:
    static constexpr int ID = 309;
    IronBootsItem() : ArmorItem(53, 2, 2, 3) {
        setTexturePosition(2, 3)->setTranslationKey("bootsIron");
    }
};

} // namespace net::minecraft::item
