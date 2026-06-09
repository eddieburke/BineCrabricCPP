#pragma once

#include "net/minecraft/item/PickaxeItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class DiamondPickaxeItem : public PickaxeItem {
public:
    static constexpr int ID = 278;
    DiamondPickaxeItem() : PickaxeItem(22, ToolMaterial::Diamond) {
        setTexturePosition(3, 6)->setTranslationKey("pickaxeDiamond");
    }
};

} // namespace net::minecraft::item
