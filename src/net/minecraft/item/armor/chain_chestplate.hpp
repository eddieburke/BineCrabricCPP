#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::item {

class ChainChestplateItem : public ArmorItem {
public:
    static constexpr int ID = 303;
    ChainChestplateItem() : ArmorItem(47, 1, 1, 1) {
        setTexturePosition(1, 1)->setTranslationKey("chestplateChain");
    }
};

} // namespace net::minecraft::item
