#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/ToolItem.hpp"
namespace net::minecraft::item {
class ShovelItem : public ToolItem {
public:
protected:
  ShovelItem(int rawId, ToolMaterial material) : ToolItem(rawId, 1, material, nullptr, 0) {}
  [[nodiscard]] float getMiningSpeedMultiplier(ItemStack* stack, Block* block) const override {
    if(block == Block::GRASS_BLOCK || block == Block::DIRT || block == Block::SAND || block == Block::GRAVEL ||
       block == Block::SNOW || block == Block::SNOW_BLOCK || block == Block::CLAY || block == Block::FARMLAND) {
      return miningSpeed_;
    }
    return ToolItem::getMiningSpeedMultiplier(stack, block);
  }
  [[nodiscard]] bool isSuitableFor(Block* block) const override {
    return block == Block::SNOW || block == Block::SNOW_BLOCK;
  }
};
} // namespace net::minecraft::item
