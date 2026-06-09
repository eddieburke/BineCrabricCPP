#pragma once

#include "net/minecraft/item/FlintAndSteel.hpp"

namespace net::minecraft::item {

class FlintAndSteelItem : public FlintAndSteel {
public:
    static constexpr int ID = 259;
    FlintAndSteelItem() : FlintAndSteel(3) {
        setTexturePosition(5, 0)->setTranslationKey("flintAndSteel");
    }
};

} // namespace net::minecraft::item
