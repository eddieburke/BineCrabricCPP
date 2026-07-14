#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/item/ToolItem.hpp"
namespace net::minecraft::item {
class PickaxeItem : public ToolItem {
public:
protected:
  PickaxeItem(int rawId, ToolMaterial material) : ToolItem(rawId, 2, material, nullptr, 0) {
  }
  [[nodiscard]] float getMiningSpeedMultiplier(ItemStack* stack, Block* block) const override {
    if(block == Block::COBBLESTONE || block == Block::DOUBLE_SLAB || block == Block::SLAB ||
       block == Block::STONE || block == Block::SANDSTONE || block == Block::MOSSY_COBBLESTONE ||
       block == Block::IRON_ORE || block == Block::IRON_BLOCK || block == Block::COAL_ORE ||
       block == Block::GOLD_BLOCK || block == Block::GOLD_ORE || block == Block::DIAMOND_ORE ||
       block == Block::DIAMOND_BLOCK || block == Block::ICE || block == Block::NETHERRACK ||
       block == Block::LAPIS_ORE || block == Block::LAPIS_BLOCK) {
      return miningSpeed_;
    }
    return ToolItem::getMiningSpeedMultiplier(stack, block);
  }
  [[nodiscard]] bool isSuitableFor(Block* block) const override {
    if(block == nullptr) {
      return false;
    }
    if(block == Block::OBSIDIAN) {
      return toolMaterialMiningLevel(toolMaterial_) == 3;
    }
    if(block == Block::DIAMOND_BLOCK || block == Block::DIAMOND_ORE || block == Block::GOLD_BLOCK ||
       block == Block::GOLD_ORE || block == Block::REDSTONE_ORE || block == Block::LIT_REDSTONE_ORE) {
      return toolMaterialMiningLevel(toolMaterial_) >= 2;
    }
    if(block == Block::IRON_BLOCK || block == Block::IRON_ORE || block == Block::LAPIS_BLOCK ||
       block == Block::LAPIS_ORE) {
      return toolMaterialMiningLevel(toolMaterial_) >= 1;
    }
    return &block->material == &block::material::Material::STONE ||
           &block->material == &block::material::Material::METAL;
  }
};
} // namespace net::minecraft::item
