#pragma once

#include "net/minecraft/item/MusicDiscItem.hpp"

namespace net::minecraft::item {

class RecordThirteenItem : public MusicDiscItem {
public:
    static constexpr int ID = 2256;
    RecordThirteenItem() : MusicDiscItem(2000, "13") {
        setTexturePosition(0, 15)->setRegistryName("record_13");
    }
};

} // namespace net::minecraft::item
