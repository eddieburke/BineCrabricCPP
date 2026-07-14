#pragma once
#include "net/minecraft/item/SnowballItem.hpp"
namespace net::minecraft::item::vanilla {
class SnowballItem : public item::SnowballItem {
public:
  static constexpr int ID = 332;
  SnowballItem() : item::SnowballItem(76) {
    setTexturePosition(14, 0)->setTranslationKey("snowball");
  }
};
} // namespace net::minecraft::item::vanilla
