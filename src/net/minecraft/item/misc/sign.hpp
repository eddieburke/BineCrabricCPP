#pragma once
#include "net/minecraft/item/SignItem.hpp"
namespace net::minecraft::item::vanilla {
class SignItem : public item::SignItem {
public:
  static constexpr int ID = 323;
  SignItem() : item::SignItem(67) {
    setTexturePosition(10, 2)->setTranslationKey("sign");
  }
};
} // namespace net::minecraft::item::vanilla
