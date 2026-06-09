#pragma once

#include "net/minecraft/item/PickaxeItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class WoodenPickaxeItem : public PickaxeItem {
public:
    static constexpr int ID = 270;
    WoodenPickaxeItem() : PickaxeItem(14, ToolMaterial::Wood) {
        setTexturePosition(0, 6)->setTranslationKey("pickaxeWood");
    }
};

} // namespace net::minecraft::item
