#pragma once

#include "net/minecraft/item/ArmorItem.hpp"

namespace net::minecraft::item {

class ChainHelmetItem : public ArmorItem {
public:
    static constexpr int ID = 302;
    ChainHelmetItem() : ArmorItem(46, 1, 1, 0) {
        setTexturePosition(1, 0)->setTranslationKey("helmetChain");
    }
};

} // namespace net::minecraft::item
