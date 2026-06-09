#pragma once

#include "net/minecraft/item/SwordItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class IronSwordItem : public SwordItem {
public:
    static constexpr int ID = 267;
    IronSwordItem() : SwordItem(11, ToolMaterial::Iron) {
        setTexturePosition(2, 4)->setTranslationKey("swordIron");
    }
};

} // namespace net::minecraft::item
