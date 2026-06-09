#pragma once

#include "net/minecraft/item/PickaxeItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class StonePickaxeItem : public PickaxeItem {
public:
    static constexpr int ID = 274;
    StonePickaxeItem() : PickaxeItem(18, ToolMaterial::Stone) {
        setTexturePosition(1, 6)->setTranslationKey("pickaxeStone");
    }
};

} // namespace net::minecraft::item
