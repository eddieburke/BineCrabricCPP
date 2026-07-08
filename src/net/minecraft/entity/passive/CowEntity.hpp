#pragma once
#include "net/minecraft/entity/EntityClientRendererDecl.hpp"
#include "net/minecraft/entity/passive/AnimalEntity.hpp"

namespace net::minecraft::entity::player {
class PlayerEntity;
}

namespace net::minecraft::entity::passive {
class CowEntity : public AnimalEntity {
   public:
    static constexpr int kEntityId = 92;
    static constexpr const char* kEntityName = "Cow";

    struct ClientRenderer {
        static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
    };

    explicit CowEntity(World* world = nullptr);
    bool interact(player::PlayerEntity* player) override;

    [[nodiscard]] std::string getRandomSound() override {
        return "mob.cow";
    }

    [[nodiscard]] std::string getHurtSound() const override {
        return "mob.cowhurt";
    }

    [[nodiscard]] std::string getDeathSound() const override {
        return "mob.cowhurt";
    }

    [[nodiscard]] float getSoundVolume() const override {
        return 0.4f;
    }

    [[nodiscard]] int getDroppedItemId() const override;
};
}  // namespace net::minecraft::entity::passive
