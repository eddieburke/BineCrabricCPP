#pragma once

#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityClientRendererDecl.hpp"

namespace net::minecraft::entity::player {
class PlayerEntity;
}

namespace net::minecraft::entity::vehicle {

class BoatEntity : public Entity {
public:
    static constexpr int kEntityId = 41;

    static constexpr const char* kEntityName = "Boat";


    struct ClientRenderer {
        static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
    };
    explicit BoatEntity(World* world = nullptr);
    BoatEntity(World* world, double x, double y, double z);

    void tick() override;
    void setPositionAndAnglesAvoidEntities(
        double x, double y, double z, float yaw, float pitch, int interpolationSteps) override;
    void setVelocityClient(double x, double y, double z) override;
    bool damage(Entity* damageSource, int amount) override;
    bool interact(player::PlayerEntity* player) override;
    void updatePassengerPosition() override;
    [[nodiscard]] bool isCollidable() const override { return !dead; }
    [[nodiscard]] bool isPushable() const override { return true; }
    [[nodiscard]] std::optional<Box> getBoundingBox() const override { return boundingBox; }
    [[nodiscard]] std::optional<Box> getCollisionAgainstShape(Entity* other) const override;
    [[nodiscard]] double getPassengerRidingHeight() const override { return static_cast<double>(height) * 0.0 - 0.3; }

    int damageWobbleTicks = 0;
    float damageWobbleStrength = 0.0f;
    int damageWobbleSide = 1;

    void onCollision(Entity* otherEntity) override;

private:
    int clientInterpolationSteps = 0;
    double clientX = 0.0;
    double clientY = 0.0;
    double clientZ = 0.0;
    double clientPitch = 0.0;
    double clientYaw = 0.0;
    double clientVelocityX = 0.0;
    double clientVelocityY = 0.0;
    double clientVelocityZ = 0.0;
};

} // namespace net::minecraft::entity::vehicle
