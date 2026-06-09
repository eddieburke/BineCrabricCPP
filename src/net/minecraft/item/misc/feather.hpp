#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class FeatherItem : public Item {
public:
    static constexpr int ID = 288;
    FeatherItem() : Item(32) {
        setTexturePosition(8, 1)->setTranslationKey("feather");
    }
};

} // namespace net::minecraft::item
