#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class GunpowderItem : public Item {
public:
    static constexpr int ID = 289;
    GunpowderItem() : Item(33) {
        setTexturePosition(8, 2)->setTranslationKey("sulphur");
    }
};

} // namespace net::minecraft::item
