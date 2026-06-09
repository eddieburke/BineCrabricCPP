#pragma once

#include "net/minecraft/item/PickaxeItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class GoldenPickaxeItem : public PickaxeItem {
public:
    static constexpr int ID = 285;
    GoldenPickaxeItem() : PickaxeItem(29, ToolMaterial::Gold) {
        setTexturePosition(4, 6)->setTranslationKey("pickaxeGold");
    }
};

} // namespace net::minecraft::item
