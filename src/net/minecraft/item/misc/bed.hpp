#pragma once
#include "net/minecraft/item/BedItem.hpp"
namespace net::minecraft::item::vanilla {
class BedItem : public item::BedItem {
public:
  static constexpr int ID = 355;
  BedItem() : item::BedItem(99) {
    setMaxCount(1);
    setTexturePosition(13, 2)->setTranslationKey("bed");
  }
};
} // namespace net::minecraft::item::vanilla
