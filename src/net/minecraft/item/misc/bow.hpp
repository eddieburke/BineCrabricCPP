#pragma once
#include "net/minecraft/item/BowItem.hpp"
namespace net::minecraft::item::vanilla {
class BowItem : public item::BowItem {
public:
  static constexpr int ID = 261;
  BowItem() : item::BowItem(5) {
    setTexturePosition(5, 1)->setTranslationKey("bow");
  }
};
} // namespace net::minecraft::item::vanilla
