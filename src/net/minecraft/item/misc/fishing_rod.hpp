#pragma once
#include "net/minecraft/item/FishingRodItem.hpp"
namespace net::minecraft::item::vanilla {
class FishingRodItem : public item::FishingRodItem {
public:
  static constexpr int ID = 346;
  FishingRodItem() : item::FishingRodItem(90) {
    setTexturePosition(5, 4)->setTranslationKey("fishingRod");
  }
};
} // namespace net::minecraft::item::vanilla
