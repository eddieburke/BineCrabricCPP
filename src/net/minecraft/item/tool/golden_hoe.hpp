#pragma once

#include "net/minecraft/item/HoeItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class GoldenHoeItem : public HoeItem {
public:
    static constexpr int ID = 294;
    GoldenHoeItem() : HoeItem(38, ToolMaterial::Gold) {
        setTexturePosition(4, 8)->setTranslationKey("hoeGold");
    }
};

} // namespace net::minecraft::item
