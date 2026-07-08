#pragma once
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityClientRendererDecl.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"

namespace net::minecraft::entity::projectile {
class FireballEntity : public Entity {
   public:
    static constexpr int kEntityId = 97;
    static constexpr const char* kEntityName = "Fireball";

    struct ClientRenderer {
        static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
    };

    explicit FireballEntity(World* world = nullptr);
    FireballEntity(
        World* world, double xIn, double yIn, double zIn, double velocityXIn, double velocityYIn, double velocityZIn);
    FireballEntity(World* world, LivingEntity* ownerIn, double velocityXIn, double velocityYIn, double velocityZIn);
    void tick() override;
    bool damage(Entity* damageSource, int amount) override;
    void writeNbt(NbtCompound& nbt) const override;
    void readNbt(const NbtCompound& nbt) override;

    [[nodiscard]] bool isCollidable() const override {
        return true;
    }

    [[nodiscard]] float getTargetingMargin() const override {
        return 1.0f;
    }

    LivingEntity* owner = nullptr;
    double powerX = 0.0;
    double powerY = 0.0;
    double powerZ = 0.0;

   private:
    int blockX = -1;
    int blockY = -1;
    int blockZ = -1;
    int blockId = 0;
    bool inGround = false;
    int shake = 0;
    int removalTimer = 0;
    int inAirTime = 0;
};
}  // namespace net::minecraft::entity::projectile
