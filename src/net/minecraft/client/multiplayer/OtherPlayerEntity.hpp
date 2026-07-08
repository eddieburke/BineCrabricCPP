#pragma once
#include <cmath>
#include <string>

#include "net/minecraft/client/auth/microsoft/MicrosoftAuth.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"

namespace net::minecraft::client::multiplayer {
class OtherPlayerEntity : public PlayerEntity {
   public:
    OtherPlayerEntity(World* world, std::string name) : PlayerEntity(world) {
        this->name = std::move(name);
        standingEyeHeight = 0.0f;
        stepHeight = 0.0f;
        if (!this->name.empty()) {
            skinUrl = msauth::legacySkinUrl(this->name);
        }
        noClip = true;
        sleepOffsetY = 0.25f;
        renderDistanceMultiplier = 10.0;
    }

    bool damage(Entity* /*damageSource*/, int /*amount*/) override {
        return true;
    }

    void setPositionAndAnglesAvoidEntities(
        double x, double y, double z, float yaw, float pitch, int interpolationSteps) override {
        targetX_ = x;
        targetY_ = y;
        targetZ_ = z;
        targetYaw_ = yaw;
        targetPitch_ = pitch;
        interpolationSteps_ = interpolationSteps;
    }

    void tick() override {
        sleepOffsetY = 0.0f;
        PlayerEntity::tick();
        lastWalkAnimationSpeed = walkAnimationSpeed;
        const double dx = x - prevX;
        const double dz = z - prevZ;
        float speed = MathHelper::sqrt(dx * dx + dz * dz) * 4.0f;
        if (speed > 1.0f) {
            speed = 1.0f;
        }
        walkAnimationSpeed += (speed - walkAnimationSpeed) * 0.4f;
        walkAnimationProgress += walkAnimationSpeed;
    }

    [[nodiscard]] float getShadowRadius() const override {
        return 0.0f;
    }

    void tickMovement() override {
        PlayerEntity::tickLiving();
        if (interpolationSteps_ > 0) {
            const double nextX = x + (targetX_ - x) / static_cast<double>(interpolationSteps_);
            const double nextY = y + (targetY_ - y) / static_cast<double>(interpolationSteps_);
            const double nextZ = z + (targetZ_ - z) / static_cast<double>(interpolationSteps_);
            double yawDelta = targetYaw_ - static_cast<double>(yaw);
            while (yawDelta < -180.0) {
                yawDelta += 360.0;
            }
            while (yawDelta >= 180.0) {
                yawDelta -= 360.0;
            }
            yaw = static_cast<float>(static_cast<double>(yaw) + yawDelta / static_cast<double>(interpolationSteps_));
            pitch = static_cast<float>(static_cast<double>(pitch) + (targetPitch_ - static_cast<double>(pitch)) /
                                                                        static_cast<double>(interpolationSteps_));
            --interpolationSteps_;
            setPosition(nextX, nextY, nextZ);
            setRotation(yaw, pitch);
        }
        prevStepBobbingAmount = stepBobbingAmount;
        float speed = MathHelper::sqrt(static_cast<float>(velocityX * velocityX + velocityZ * velocityZ));
        float bob = static_cast<float>(std::atan(-velocityY * 0.2) * 15.0);
        if (speed > 0.1f) {
            speed = 0.1f;
        }
        if (!onGround || health <= 0) {
            speed = 0.0f;
        }
        if (onGround || health <= 0) {
            bob = 0.0f;
        }
        stepBobbingAmount += (speed - stepBobbingAmount) * 0.4f;
        tilt += (bob - tilt) * 0.8f;
    }

    void setEquipmentStack(int armorSlot, int itemId, int meta) override {
        ItemStack stack;
        if (itemId >= 0) {
            stack = ItemStack(itemId, 1, meta);
        }
        if (armorSlot == 0) {
            inventory.main[static_cast<std::size_t>(inventory.selectedSlot)] = stack;
        } else if (armorSlot > 0 && armorSlot <= static_cast<int>(inventory.armor.size())) {
            inventory.armor[static_cast<std::size_t>(armorSlot - 1)] = stack;
        }
    }

    void spawn() {
    }

   private:
    int interpolationSteps_ = 0;
    double targetX_ = 0.0;
    double targetY_ = 0.0;
    double targetZ_ = 0.0;
    double targetYaw_ = 0.0;
    double targetPitch_ = 0.0;
};
}  // namespace net::minecraft::client::multiplayer
