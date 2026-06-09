#pragma once

#include "net/minecraft/item/AxeItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class IronAxeItem : public AxeItem {
public:
    static constexpr int ID = 258;
    IronAxeItem() : AxeItem(2, ToolMaterial::Iron) {
        setTexturePosition(2, 7)->setTranslationKey("hatchetIron");
    }
};

} // namespace net::minecraft::item
