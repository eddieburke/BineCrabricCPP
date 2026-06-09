#pragma once

#include "net/minecraft/item/PaintingItem.hpp"

namespace net::minecraft::item::vanilla {

class PaintingItem : public item::PaintingItem {
public:
    static constexpr int ID = 321;
    PaintingItem() : item::PaintingItem(65) {
        setTexturePosition(10, 1)->setTranslationKey("painting");
    }
};

} // namespace net::minecraft::item::vanilla
