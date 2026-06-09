#pragma once

#include "net/minecraft/item/HoeItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class StoneHoeItem : public HoeItem {
public:
    static constexpr int ID = 291;
    StoneHoeItem() : HoeItem(35, ToolMaterial::Stone) {
        setTexturePosition(1, 8)->setTranslationKey("hoeStone");
    }
};

} // namespace net::minecraft::item
