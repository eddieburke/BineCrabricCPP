#pragma once

#include "net/minecraft/item/ShovelItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class GoldenShovelItem : public ShovelItem {
public:
    static constexpr int ID = 284;
    GoldenShovelItem() : ShovelItem(28, ToolMaterial::Gold) {
        setTexturePosition(4, 5)->setTranslationKey("shovelGold");
    }
};

} // namespace net::minecraft::item
