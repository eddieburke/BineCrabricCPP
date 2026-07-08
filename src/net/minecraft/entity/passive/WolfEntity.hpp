#pragma once
#include "net/minecraft/entity/EntityClientRendererDecl.hpp"
#include "net/minecraft/entity/passive/AnimalEntity.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"

namespace net::minecraft::entity::player {
class PlayerEntity;
}

namespace net::minecraft::entity::passive {
class WolfEntity : public AnimalEntity {
   public:
    static constexpr int kEntityId = 95;
    static constexpr const char* kEntityName = "Wolf";

    struct ClientRenderer {
        static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
    };

    explicit WolfEntity(World* world = nullptr);
    bool begging = false;
    float begAnimationProgress = 0.0f;
    float lastBegAnimationProcess = 0.0f;
    bool furWet = false;
    bool shakingWaterOff = false;
    float shakeProgress = 0.0f;
    float lastShakeProgress = 0.0f;

    void initDataTracker() override {
        LivingEntity::initDataTracker();
        dataTracker.startTracking(16, static_cast<std::int8_t>(0));
        dataTracker.startTracking(17, std::string{});
        dataTracker.startTracking(18, health);
    }

    void tick() override;
    void tickMovement() override;
    bool damage(Entity* damageSource, int amount) override;
    bool interact(player::PlayerEntity* player) override;
    void writeNbt(NbtCompound& nbt) const override;
    void readNbt(const NbtCompound& nbt) override;
    void processServerEntityStatus(std::int8_t status) override;

    [[nodiscard]] bool isAngry() const {
        return (dataTracker.getByte(16) & 2) != 0;
    }

    void setAngry(bool angry) {
        std::int8_t flags = dataTracker.getByte(16);
        if (angry) {
            dataTracker.set(16, static_cast<std::int8_t>(flags | 2));
        } else {
            dataTracker.set(16, static_cast<std::int8_t>(flags & 0xFFFFFFFD));
        }
    }

    [[nodiscard]] bool isInSittingPose() const {
        return (dataTracker.getByte(16) & 1) != 0;
    }

    void setSitting(bool inSittingPose) {
        std::int8_t flags = dataTracker.getByte(16);
        if (inSittingPose) {
            dataTracker.set(16, static_cast<std::int8_t>(flags | 1));
        } else {
            dataTracker.set(16, static_cast<std::int8_t>(flags & 0xFFFFFFFE));
        }
    }

    [[nodiscard]] bool isTamed() const {
        return (dataTracker.getByte(16) & 4) != 0;
    }

    void setTamed(bool tamed) {
        std::int8_t flags = dataTracker.getByte(16);
        if (tamed) {
            dataTracker.set(16, static_cast<std::int8_t>(flags | 4));
        } else {
            dataTracker.set(16, static_cast<std::int8_t>(flags & 0xFFFFFFFB));
        }
    }

    [[nodiscard]] std::string getOwnerName() const {
        return dataTracker.getString(17);
    }

    void setOwnerName(const std::string& owner) {
        dataTracker.set(17, owner);
    }

    [[nodiscard]] bool isFurWet() const {
        return furWet;
    }

    [[nodiscard]] float getFurBrightnessMultiplier(float tickDelta) const {
        return 0.75f + (lastShakeProgress + (shakeProgress - lastShakeProgress) * tickDelta) / 2.0f * 0.25f;
    }

    [[nodiscard]] float getShakeAnimationProgress(float tickDelta, float offset) const {
        float f = (lastShakeProgress + (shakeProgress - lastShakeProgress) * tickDelta + offset) / 1.8f;
        f = std::clamp(f, 0.0f, 1.0f);
        return MathHelper::sin(f * kPiF) * MathHelper::sin(f * kPiF * 11.0f) * 0.15f * kPiF;
    }

    [[nodiscard]] float getBegAnimationProgress(float tickDelta) const {
        return (lastBegAnimationProcess + (begAnimationProgress - lastBegAnimationProcess) * tickDelta) * 0.15f * kPiF;
    }

    [[nodiscard]] float getTailAngle() const {
        if (isAngry()) {
            return 1.5393804f;
        }
        if (isTamed()) {
            return (0.55f - static_cast<float>(20 - dataTracker.getInt(18)) * 0.02f) * kPiF;
        }
        return 0.62831855f;
    }

    [[nodiscard]] std::string getTexture() const override {
        if (isTamed()) {
            return "/mob/wolf_tame.png";
        }
        if (isAngry()) {
            return "/mob/wolf_angry.png";
        }
        return texture;
    }

    [[nodiscard]] std::string getRandomSound() override;

    [[nodiscard]] std::string getHurtSound() const override {
        return "mob.wolf.hurt";
    }

    [[nodiscard]] std::string getDeathSound() const override {
        return "mob.wolf.death";
    }

    [[nodiscard]] float getSoundVolume() const override {
        return 0.4f;
    }

    [[nodiscard]] float getEyeHeight() const override {
        return height * 0.8f;
    }

    [[nodiscard]] int getDroppedItemId() const override;

    [[nodiscard]] int getLimitPerChunk() const override {
        return 8;
    }

    [[nodiscard]] bool bypassesSteppingEffects() const override {
        return false;
    }

   protected:
    [[nodiscard]] bool canDespawn() const override {
        return !isTamed();
    }

    [[nodiscard]] bool isMovementBlocked() const override;
    Entity* getTargetInRange() override;
    void attack(Entity* other, float distance) override;
    void tickLiving() override;
    [[nodiscard]] int getMaxLookPitchChange() const override;

   private:
    void followOwner(float distance);
    void showFeedParticles(bool hearts);
    [[nodiscard]] static bool namesEqualIgnoreCase(const std::string& left, const std::string& right);
};
}  // namespace net::minecraft::entity::passive
