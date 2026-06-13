#pragma once

#include "net/minecraft/entity/mob/MonsterEntity.hpp"
#include "net/minecraft/entity/EntityClientRendererDecl.hpp"

namespace net::minecraft::entity::mob {

class SpiderEntity : public MonsterEntity {
public:
    static constexpr int kEntityId = 52;

    static constexpr const char* kEntityName = "Spider";


    struct ClientRenderer {
        static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
    };
    explicit SpiderEntity(World* world = nullptr);

    [[nodiscard]] double getPassengerRidingHeight() const override
    {
        return static_cast<double>(height) * 0.75 - 0.5;
    }

    [[nodiscard]] bool isOnLadder() const override { return horizontalCollision; }

    [[nodiscard]] bool bypassesSteppingEffects() const override { return false; }

protected:
    Entity* getTargetInRange() override;
    void attack(Entity* other, float distance) override;

    [[nodiscard]] std::string getRandomSound() override { return "mob.spider"; }
    [[nodiscard]] std::string getHurtSound() const override { return "mob.spider"; }
    [[nodiscard]] std::string getDeathSound() const override { return "mob.spiderdeath"; }

    [[nodiscard]] int getDroppedItemId() const override;
};

} // namespace net::minecraft::entity::mob
