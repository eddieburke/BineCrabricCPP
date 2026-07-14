#pragma once
#include "net/minecraft/item/DyeItem.hpp"
namespace net::minecraft::item::vanilla {
class DyeItem : public item::DyeItem {
public:
  static constexpr int ID = 351;
  DyeItem() : item::DyeItem(95) {
    setTexturePosition(14, 4)->setTranslationKey("dyePowder");
  }
};
} // namespace net::minecraft::item::vanilla
