#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class WheatItem : public Item {
public:
    static constexpr int ID = 296;
    WheatItem() : Item(40) {
        setTexturePosition(9, 1)->setTranslationKey("wheat");
    }
};

} // namespace net::minecraft::item
