#pragma once

#include "net/minecraft/item/SwordItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class StoneSwordItem : public SwordItem {
public:
    static constexpr int ID = 272;
    StoneSwordItem() : SwordItem(16, ToolMaterial::Stone) {
        setTexturePosition(1, 4)->setTranslationKey("swordStone");
    }
};

} // namespace net::minecraft::item
