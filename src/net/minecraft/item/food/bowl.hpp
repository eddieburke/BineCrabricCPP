#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class BowlItem : public Item {
public:
    static constexpr int ID = 281;
    BowlItem() : Item(25) {
        setTexturePosition(7, 4)->setTranslationKey("bowl");
    }
};

} // namespace net::minecraft::item
