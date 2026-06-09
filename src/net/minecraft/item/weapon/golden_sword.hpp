#pragma once

#include "net/minecraft/item/SwordItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class GoldenSwordItem : public SwordItem {
public:
    static constexpr int ID = 283;
    GoldenSwordItem() : SwordItem(27, ToolMaterial::Gold) {
        setTexturePosition(4, 4)->setTranslationKey("swordGold");
    }
};

} // namespace net::minecraft::item
