#pragma once

#include "net/minecraft/item/EggItem.hpp"

namespace net::minecraft::item::vanilla {

class EggItem : public item::EggItem {
public:
    static constexpr int ID = 344;
    EggItem() : item::EggItem(88) {
        setTexturePosition(12, 0)->setTranslationKey("egg");
    }
};

} // namespace net::minecraft::item::vanilla
