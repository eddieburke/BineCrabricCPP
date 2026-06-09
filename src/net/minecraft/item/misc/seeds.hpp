#pragma once

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/SeedsItem.hpp"

namespace net::minecraft::item::vanilla {

class SeedsItem : public item::SeedsItem {
public:
    static constexpr int ID = 295;
    SeedsItem() : item::SeedsItem(39, Block::WHEAT != nullptr ? Block::WHEAT->id : 59) {
        setTexturePosition(9, 0)->setTranslationKey("seeds");
    }
};

} // namespace net::minecraft::item::vanilla
