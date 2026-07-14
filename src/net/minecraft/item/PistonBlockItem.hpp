#pragma once
#include "net/minecraft/item/BlockItem.hpp"
namespace net::minecraft::item {
class PistonBlockItem : public BlockItem {
public:
  explicit PistonBlockItem(int rawId) : BlockItem(rawId) {
  }
  [[nodiscard]] int getPlacementMetadata(int /*meta*/) const override {
    return 7;
  }
};
} // namespace net::minecraft::item
