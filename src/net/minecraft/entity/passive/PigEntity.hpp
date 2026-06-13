#pragma once

#include "net/minecraft/entity/passive/AnimalEntity.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/entity/EntityClientRendererDecl.hpp"

namespace net::minecraft::entity::player {
class PlayerEntity;
}

namespace net::minecraft::entity::passive {

class PigEntity : public AnimalEntity {
public:
    static constexpr int kEntityId = 90;

    static constexpr const char* kEntityName = "Pig";


    struct ClientRenderer {
        static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
    };
    explicit PigEntity(World* world = nullptr);

    void initDataTracker() override
    {
        dataTracker.startTracking(16, static_cast<std::int8_t>(0));
    }

    void writeNbt(NbtCompound& nbt) const override;
    void readNbt(const NbtCompound& nbt) override;
    bool interact(player::PlayerEntity* player) override;
    void onStruckByLightning(Entity* lightning) override;
    void onLanding(float fallDistance) override;

    [[nodiscard]] bool isSaddled() const
    {
        return (dataTracker.getByte(16) & 1) != 0;
    }

    void setSaddled(bool value)
    {
        dataTracker.set(16, static_cast<std::int8_t>(value ? 1 : 0));
    }

    [[nodiscard]] std::string getRandomSound() override { return "mob.pig"; }
    [[nodiscard]] std::string getHurtSound() const override { return "mob.pig"; }
    [[nodiscard]] std::string getDeathSound() const override { return "mob.pigdeath"; }

    [[nodiscard]] int getDroppedItemId() const override;
};

} // namespace net::minecraft::entity::passive
