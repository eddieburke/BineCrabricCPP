#pragma once

#include "net/minecraft/item/ShovelItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class IronShovelItem : public ShovelItem {
public:
    static constexpr int ID = 256;
    IronShovelItem() : ShovelItem(0, ToolMaterial::Iron) {
        setTexturePosition(2, 5)->setTranslationKey("shovelIron");
    }
};

} // namespace net::minecraft::item
