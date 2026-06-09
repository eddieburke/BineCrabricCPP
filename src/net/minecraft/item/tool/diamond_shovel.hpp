#pragma once

#include "net/minecraft/item/ShovelItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class DiamondShovelItem : public ShovelItem {
public:
    static constexpr int ID = 277;
    DiamondShovelItem() : ShovelItem(21, ToolMaterial::Diamond) {
        setTexturePosition(3, 5)->setTranslationKey("shovelDiamond");
    }
};

} // namespace net::minecraft::item
