#pragma once

#include "net/minecraft/item/ShovelItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class StoneShovelItem : public ShovelItem {
public:
    static constexpr int ID = 273;
    StoneShovelItem() : ShovelItem(17, ToolMaterial::Stone) {
        setTexturePosition(1, 5)->setTranslationKey("shovelStone");
    }
};

} // namespace net::minecraft::item
