#pragma once

#include "net/minecraft/item/AxeItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class StoneAxeItem : public AxeItem {
public:
    static constexpr int ID = 275;
    StoneAxeItem() : AxeItem(19, ToolMaterial::Stone) {
        setTexturePosition(1, 7)->setTranslationKey("hatchetStone");
    }
};

} // namespace net::minecraft::item
