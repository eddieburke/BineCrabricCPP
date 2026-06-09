#pragma once

#include "net/minecraft/item/ShearsItem.hpp"

namespace net::minecraft::item::vanilla {

class ShearsItem : public item::ShearsItem {
public:
    static constexpr int ID = 359;
    ShearsItem() : item::ShearsItem(103) {
        setTexturePosition(13, 5)->setTranslationKey("shears");
    }
};

} // namespace net::minecraft::item::vanilla
