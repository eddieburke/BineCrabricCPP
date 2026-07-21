#pragma once
#include "net/minecraft/item/Item.hpp"
namespace net::minecraft {
class World;
class ItemStack;
} // namespace net::minecraft
namespace net::minecraft::item {
class SnowballItem : public Item {
 public:
 static constexpr int kRawId = 76;
 static void registerClass();
 explicit SnowballItem(int rawId);
 ItemStack* use(ItemStack* stack, World* world, PlayerEntity* user) override;
};
} // namespace net::minecraft::item
