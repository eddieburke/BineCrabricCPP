#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class SugarItem : public Item {
public:
    static constexpr int ID = 353;
    SugarItem() : Item(97) {
        setHandheld();
        setTexturePosition(13, 0)->setTranslationKey("sugar");
    }
};

} // namespace net::minecraft::item
