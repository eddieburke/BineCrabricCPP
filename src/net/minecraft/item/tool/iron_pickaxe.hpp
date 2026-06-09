#pragma once

#include "net/minecraft/item/PickaxeItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class IronPickaxeItem : public PickaxeItem {
public:
    static constexpr int ID = 257;
    IronPickaxeItem() : PickaxeItem(1, ToolMaterial::Iron) {
        setTexturePosition(2, 6)->setTranslationKey("pickaxeIron");
    }
};

} // namespace net::minecraft::item
