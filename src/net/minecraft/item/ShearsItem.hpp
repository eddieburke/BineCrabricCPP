#pragma once

#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/item/Item.hpp"

namespace net::minecraft::item {

class ShearsItem : public Item {
public:
    explicit ShearsItem(int rawId);
    bool postMine(ItemStack* stack, int blockId, int x, int y, int z, LivingEntity* miner) override;
    [[nodiscard]] bool isSuitableFor(Block* block) const override;
    [[nodiscard]] float getMiningSpeedMultiplier(ItemStack* stack, Block* block) const override;
};

} // namespace net::minecraft::item
