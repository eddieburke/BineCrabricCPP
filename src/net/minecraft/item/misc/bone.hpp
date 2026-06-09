#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class BoneItem : public Item {
public:
    static constexpr int ID = 352;
    BoneItem() : Item(96) {
        setHandheld();
        setTexturePosition(12, 1)->setTranslationKey("bone");
    }
};

} // namespace net::minecraft::item
