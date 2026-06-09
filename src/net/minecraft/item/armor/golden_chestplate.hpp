#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::item {

class GoldenChestplateItem : public ArmorItem {
public:
    static constexpr int ID = 315;
    GoldenChestplateItem() : ArmorItem(59, 1, 4, 1) {
        setTexturePosition(4, 1)->setTranslationKey("chestplateGold");
    }
};

} // namespace net::minecraft::item
