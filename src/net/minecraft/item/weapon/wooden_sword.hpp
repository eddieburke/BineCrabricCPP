#pragma once

#include "net/minecraft/item/SwordItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class WoodenSwordItem : public SwordItem {
public:
    static constexpr int ID = 268;
    WoodenSwordItem() : SwordItem(12, ToolMaterial::Wood) {
        setTexturePosition(0, 4)->setTranslationKey("swordWood");
    }
};

} // namespace net::minecraft::item
