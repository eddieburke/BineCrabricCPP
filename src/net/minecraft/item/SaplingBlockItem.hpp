#pragma once
#include "net/minecraft/item/BlockItem.hpp"
namespace net::minecraft::item {
class SaplingBlockItem : public BlockItem {
public:
  explicit SaplingBlockItem(int rawId) : BlockItem(rawId) {
    setMaxDamage(0);
    setHasSubtypes(true);
  }
  [[nodiscard]] int getPlacementMetadata(int meta) const override {
    return meta;
  }
  [[nodiscard]] int getTextureId(int damage) const override {
    return Block::SAPLING != nullptr ? Block::SAPLING->getTexture(0, damage) : BlockItem::getTextureId(damage);
  }
};
} // namespace net::minecraft::item
