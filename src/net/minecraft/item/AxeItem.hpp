#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/ToolItem.hpp"

namespace net::minecraft::item {
class AxeItem : public ToolItem {
   public:
   protected:
    AxeItem(int rawId, ToolMaterial material) : ToolItem(rawId, 3, material, nullptr, 0) {
    }

    [[nodiscard]] float getMiningSpeedMultiplier(ItemStack* stack, Block* block) const override {
        if (block == Block::PLANKS || block == Block::BOOKSHELF || block == Block::LOG || block == Block::CHEST) {
            return miningSpeed_;
        }
        return ToolItem::getMiningSpeedMultiplier(stack, block);
    }
};
}  // namespace net::minecraft::item
