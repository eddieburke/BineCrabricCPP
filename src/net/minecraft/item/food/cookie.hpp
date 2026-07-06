#pragma once
#include "net/minecraft/item/StackableFoodItem.hpp"
namespace net::minecraft::item {
class CookieItem : public StackableFoodItem {
public:
  static constexpr int ID = 357;
  CookieItem() : StackableFoodItem(101, 1, false, 8) {
    setTexturePosition(12, 5)->setTranslationKey("cookie");
  }
};
} // namespace net::minecraft::item
