#pragma once

#include "net/minecraft/item/HoeItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class DiamondHoeItem : public HoeItem {
public:
    static constexpr int ID = 293;
    DiamondHoeItem() : HoeItem(37, ToolMaterial::Diamond) {
        setTexturePosition(3, 8)->setTranslationKey("hoeDiamond");
    }
};

} // namespace net::minecraft::item
