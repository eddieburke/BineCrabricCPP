#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class GlowstoneDustItem : public Item {
public:
    static constexpr int ID = 348;
    GlowstoneDustItem() : Item(92) {
        setTexturePosition(9, 4)->setTranslationKey("yellowDust");
    }
};

} // namespace net::minecraft::item
