#pragma once
#include "net/minecraft/item/MusicDiscItem.hpp"

namespace net::minecraft::item {
class RecordCatItem : public MusicDiscItem {
   public:
    static constexpr int ID = 2257;

    RecordCatItem() : MusicDiscItem(2001, "cat") {
        setTexturePosition(1, 15)->setRegistryName("record_cat");
    }
};
}  // namespace net::minecraft::item
