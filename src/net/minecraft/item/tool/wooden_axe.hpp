#pragma once

#include "net/minecraft/item/AxeItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class WoodenAxeItem : public AxeItem {
public:
    static constexpr int ID = 271;
    WoodenAxeItem() : AxeItem(15, ToolMaterial::Wood) {
        setTexturePosition(0, 7)->setTranslationKey("hatchetWood");
    }
};

} // namespace net::minecraft::item
