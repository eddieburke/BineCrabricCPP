#pragma once
#include "net/minecraft/item/ToolItem.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {
class HoeItem : public ToolItem {
   public:
   protected:
    HoeItem(int rawId, ToolMaterial material);
    bool useOnBlock(ItemStack* stack, PlayerEntity* user, World* world, int x, int y, int z, int side) override;
};
}  // namespace net::minecraft::item
