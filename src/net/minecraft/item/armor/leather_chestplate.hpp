#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::item {

class LeatherChestplateItem : public ArmorItem {
public:
    static constexpr int ID = 299;
    LeatherChestplateItem() : ArmorItem(43, 0, 0, 1) {
        setTexturePosition(0, 1)->setTranslationKey("chestplateCloth");
    }
};

} // namespace net::minecraft::item
