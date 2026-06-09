#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class BookItem : public Item {
public:
    static constexpr int ID = 340;
    BookItem() : Item(84) {
        setTexturePosition(11, 3)->setTranslationKey("book");
    }
};

} // namespace net::minecraft::item
