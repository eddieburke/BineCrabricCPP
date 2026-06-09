#pragma once

#include "net/minecraft/item/AxeItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class DiamondAxeItem : public AxeItem {
public:
    static constexpr int ID = 279;
    DiamondAxeItem() : AxeItem(23, ToolMaterial::Diamond) {
        setTexturePosition(3, 7)->setTranslationKey("hatchetDiamond");
    }
};

} // namespace net::minecraft::item
