#pragma once

#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/item/ToolMaterial.hpp"

#include <vector>

namespace net::minecraft::item {

class ToolItem : public Item {
public:
protected:
    ToolItem(int rawId, int damageBoost, ToolMaterial material, Block** effectiveOn, int effectiveCount)
        : Item(rawId, RegistrationMode::Deferred),
          toolMaterial_(material),
          miningSpeed_(toolMaterialMiningSpeed(material)),
          damage_(damageBoost + toolMaterialAttackDamage(material))
    {
        setMaxCount(1);
        setMaxDamage(toolMaterialDurability(material));
        for (int i = 0; effectiveOn != nullptr && i < effectiveCount; ++i) {
            effectiveOnBlocks_.push_back(effectiveOn[i]);
        }
    }

    [[nodiscard]] float getMiningSpeedMultiplier(ItemStack* /*stack*/, Block* block) const override
    {
        for (Block* effective : effectiveOnBlocks_) {
            if (effective == block) {
                return miningSpeed_;
            }
        }
        return 1.0f;
    }

    bool postHit(ItemStack* stack, LivingEntity* target, LivingEntity* attacker) override;
    bool postMine(ItemStack* stack, int blockId, int x, int y, int z, LivingEntity* miner) override;

    [[nodiscard]] int getAttackDamage(Entity* /*attacked*/) const override { return damage_; }

protected:
    ToolMaterial toolMaterial_ = ToolMaterial::Wood;
    float miningSpeed_ = 4.0f;
    int damage_ = 0;
    std::vector<Block*> effectiveOnBlocks_;
};

} // namespace net::minecraft::item
