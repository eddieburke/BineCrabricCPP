#pragma once
#include "net/minecraft/block/SlabBlock.hpp"
#include "net/minecraft/item/BlockItem.hpp"
namespace net::minecraft::item {
class SlabBlockItem : public BlockItem {
public:
  explicit SlabBlockItem(int rawId) : BlockItem(rawId) {
    setMaxDamage(0);
    setHasSubtypes(true);
  }
  [[nodiscard]] int getPlacementMetadata(int meta) const override {
    return meta;
  }
  [[nodiscard]] int getTextureId(int damage) const override {
    return Block::SLAB != nullptr ? Block::SLAB->getTexture(2, damage) : BlockItem::getTextureId(damage);
  }
  [[nodiscard]] std::string getTranslationKey(const ItemStack* stack) const override {
    const int damage = stack != nullptr ? stack->getDamage() : 0;
    const auto index = static_cast<std::size_t>(damage) % block::SlabBlock::names.size();
    return BlockItem::getTranslationKey(stack) + "." + std::string(block::SlabBlock::names[index]);
  }
};
} // namespace net::minecraft::item
namespace net::minecraft {
using SlabBlockItem = item::SlabBlockItem;
}
