#pragma once

#include "net/minecraft/item/HoeItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class WoodenHoeItem : public HoeItem {
public:
    static constexpr int ID = 290;
    WoodenHoeItem() : HoeItem(34, ToolMaterial::Wood) {
        setTexturePosition(0, 8)->setTranslationKey("hoeWood");
    }
};

} // namespace net::minecraft::item
