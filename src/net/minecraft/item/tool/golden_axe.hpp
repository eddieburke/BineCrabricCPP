#pragma once

#include "net/minecraft/item/AxeItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class GoldenAxeItem : public AxeItem {
public:
    static constexpr int ID = 286;
    GoldenAxeItem() : AxeItem(30, ToolMaterial::Gold) {
        setTexturePosition(4, 7)->setTranslationKey("hatchetGold");
    }
};

} // namespace net::minecraft::item
