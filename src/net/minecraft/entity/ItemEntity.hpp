#pragma once

#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/item/ItemStack.hpp"

namespace net::minecraft::entity::player {
class PlayerEntity;
}

namespace net::minecraft::entity {

class ItemEntity : public Entity {
public:
    ItemStack stack {};
    int itemAge = 0;
    int pickupDelay = 0;
    float initialRotationAngle = 0.0f;

    void tick() override;
    void onPlayerInteraction(player::PlayerEntity* player) override;
    bool damage(Entity* damageSource, int amount) override;
    [[nodiscard]] bool checkWaterCollisions();
    [[nodiscard]] bool bypassesSteppingEffects() const override { return false; }

    explicit ItemEntity(World* world = nullptr)
        : Entity(world)
    {
        setBoundingBoxSpacing(0.25f, 0.25f);
        standingEyeHeight = height / 2.0f;
        initialRotationAngle = static_cast<float>(random.nextDouble() * 3.141592653589793 * 2.0);
    }

    ItemEntity(World* world, double x, double y, double z, ItemStack stackIn)
        : Entity(world)
    {
        setBoundingBoxSpacing(0.25f, 0.25f);
        standingEyeHeight = height / 2.0f;
        initialRotationAngle = static_cast<float>(random.nextDouble() * 3.141592653589793 * 2.0);
        setPosition(x, y, z);
        stack = std::move(stackIn);
        yaw = static_cast<float>(random.nextDouble() * 360.0);
        velocityX = random.nextDouble() * 0.2 - 0.1;
        velocityY = 0.2;
        velocityZ = random.nextDouble() * 0.2 - 0.1;
    }

    void writeNbt(NbtCompound& nbt) const override
    {
        Entity::writeNbt(nbt);
        nbt.putShort("Health", static_cast<std::int16_t>(health_));
        nbt.putShort("Age", static_cast<std::int16_t>(itemAge));
        nbt.put("Item", stack.toNbt());
    }

    void readNbt(const NbtCompound& nbt) override
    {
        Entity::readNbt(nbt);
        health_ = nbt.getShort("Health") & 0xFF;
        itemAge = nbt.getShort("Age");
        if (nbt.contains("Item")) {
            stack = ItemStack::fromNbt(nbt.getCompound("Item"));
        }
    }

private:
    int itemTicks = 0;
    int health_ = 5;
};

} // namespace net::minecraft::entity
