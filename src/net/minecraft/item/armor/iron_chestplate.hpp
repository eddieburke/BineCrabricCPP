#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::item {

class IronChestplateItem : public ArmorItem {
public:
    static constexpr int ID = 307;
    IronChestplateItem() : ArmorItem(51, 2, 2, 1) {
        setTexturePosition(2, 1)->setTranslationKey("chestplateIron");
    }
};

} // namespace net::minecraft::item
