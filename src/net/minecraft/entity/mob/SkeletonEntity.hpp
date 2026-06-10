#pragma once

#include "net/minecraft/entity/mob/MonsterEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemStack.hpp"

namespace net::minecraft::entity::mob {

class SkeletonEntity : public MonsterEntity {
public:
    static void registerClass();
    explicit SkeletonEntity(World* world = nullptr);

    void tickMovement() override;

    [[nodiscard]] std::string getRandomSound() override { return "mob.skeleton"; }
    [[nodiscard]] std::string getHurtSound() const override { return "mob.skeletonhurt"; }
    [[nodiscard]] std::string getDeathSound() const override { return "mob.skeletonhurt"; }

    [[nodiscard]] ItemStack getHeldItem() const override
    {
        return ItemStack(Item::BOW, 1);
    }

protected:
    void attack(Entity* other, float distance) override;
    void dropItems() override;
};

} // namespace net::minecraft::entity::mob
