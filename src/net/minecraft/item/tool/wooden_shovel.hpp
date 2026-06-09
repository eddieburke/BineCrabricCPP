#pragma once

#include "net/minecraft/item/ShovelItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class WoodenShovelItem : public ShovelItem {
public:
    static constexpr int ID = 269;
    WoodenShovelItem() : ShovelItem(13, ToolMaterial::Wood) {
        setTexturePosition(0, 5)->setTranslationKey("shovelWood");
    }
};

} // namespace net::minecraft::item
