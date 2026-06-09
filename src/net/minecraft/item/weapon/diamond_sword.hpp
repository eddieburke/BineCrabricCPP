#pragma once

#include "net/minecraft/item/SwordItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class DiamondSwordItem : public SwordItem {
public:
    static constexpr int ID = 276;
    DiamondSwordItem() : SwordItem(20, ToolMaterial::Diamond) {
        setTexturePosition(3, 4)->setTranslationKey("swordDiamond");
    }
};

} // namespace net::minecraft::item
