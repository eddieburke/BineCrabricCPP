#pragma once
#include "net/minecraft/item/RedstoneItem.hpp"
namespace net::minecraft::item::vanilla {
class RedstoneItem : public item::RedstoneItem {
public:
  static constexpr int ID = 331;
  RedstoneItem() : item::RedstoneItem(75) {
    setTexturePosition(8, 3)->setTranslationKey("redstone");
  }
};
} // namespace net::minecraft::item::vanilla
