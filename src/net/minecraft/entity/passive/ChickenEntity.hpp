#pragma once

#include "net/minecraft/entity/passive/AnimalEntity.hpp"
#include "net/minecraft/entity/EntityClientRendererDecl.hpp"

namespace net::minecraft::entity::passive {

class ChickenEntity : public AnimalEntity {
public:
    static constexpr int kEntityId = 93;

    static constexpr const char* kEntityName = "Chicken";


    struct ClientRenderer {
        static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
    };
    explicit ChickenEntity(World* world = nullptr);

    bool unused = false;
    float flapProgress = 0.0f;
    float maxWingDeviation = 0.0f;
    float prevMaxWingDeviation = 0.0f;
    float prevFlapProgress = 0.0f;
    float flapSpeed = 1.0f;
    int eggLayTime = 0;

    void tickMovement() override;
    void onLanding(float /*fallDistance*/) override {}

    [[nodiscard]] std::string getRandomSound() override { return "mob.chicken"; }
    [[nodiscard]] std::string getHurtSound() const override { return "mob.chickenhurt"; }
    [[nodiscard]] std::string getDeathSound() const override { return "mob.chickenhurt"; }

    [[nodiscard]] int getDroppedItemId() const override;
};

} // namespace net::minecraft::entity::passive
