#pragma once

#include "net/minecraft/item/Item.hpp"

namespace net::minecraft {
class World;
class ItemStack;
} // namespace net::minecraft

namespace net::minecraft::item {

class FishingRodItem : public Item {
public:
    explicit FishingRodItem(int rawId);
    [[nodiscard]] bool isHandheldRod() const override;
    ItemStack* use(ItemStack* stack, World* world, PlayerEntity* user) override;
};

} // namespace net::minecraft::item
