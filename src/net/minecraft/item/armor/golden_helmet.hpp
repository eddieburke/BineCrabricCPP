#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::item {

class GoldenHelmetItem : public ArmorItem {
public:
    static constexpr int ID = 314;
    GoldenHelmetItem() : ArmorItem(58, 1, 4, 0) {
        setTexturePosition(4, 0)->setTranslationKey("helmetGold");
    }
};

} // namespace net::minecraft::item
