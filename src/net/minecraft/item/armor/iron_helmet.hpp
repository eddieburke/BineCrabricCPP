#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::item {

class IronHelmetItem : public ArmorItem {
public:
    static constexpr int ID = 306;
    IronHelmetItem() : ArmorItem(50, 2, 2, 0) {
        setTexturePosition(2, 0)->setTranslationKey("helmetIron");
    }
};

} // namespace net::minecraft::item
