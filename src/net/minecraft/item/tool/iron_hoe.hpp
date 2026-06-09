#pragma once

#include "net/minecraft/item/HoeItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class IronHoeItem : public HoeItem {
public:
    static constexpr int ID = 292;
    IronHoeItem() : HoeItem(36, ToolMaterial::Iron) {
        setTexturePosition(2, 8)->setTranslationKey("hoeIron");
    }
};

} // namespace net::minecraft::item
