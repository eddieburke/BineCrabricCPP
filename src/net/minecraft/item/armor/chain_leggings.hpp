#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::item {

class ChainLeggingsItem : public ArmorItem {
public:
    static constexpr int ID = 304;
    ChainLeggingsItem() : ArmorItem(48, 1, 1, 2) {
        setTexturePosition(1, 2)->setTranslationKey("leggingsChain");
    }
};

} // namespace net::minecraft::item
