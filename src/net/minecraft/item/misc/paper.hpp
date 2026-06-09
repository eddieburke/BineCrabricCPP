#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class PaperItem : public Item {
public:
    static constexpr int ID = 339;
    PaperItem() : Item(83) {
        setTexturePosition(10, 3)->setTranslationKey("paper");
    }
};

} // namespace net::minecraft::item
