#pragma once

#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

namespace net::minecraft::item {

class HoeItem : public Item {
public:
    static void registerClass();
    HoeItem(int rawId, ToolMaterial material);
    bool useOnBlock(ItemStack* stack, PlayerEntity* user, World* world, int x, int y, int z, int side) override;
};

} // namespace net::minecraft::item
