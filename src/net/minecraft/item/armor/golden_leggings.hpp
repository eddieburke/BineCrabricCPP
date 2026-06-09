#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::item {

class GoldenLeggingsItem : public ArmorItem {
public:
    static constexpr int ID = 316;
    GoldenLeggingsItem() : ArmorItem(60, 1, 4, 2) {
        setTexturePosition(4, 2)->setTranslationKey("leggingsGold");
    }
};

} // namespace net::minecraft::item
